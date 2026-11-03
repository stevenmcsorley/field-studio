#!/usr/bin/env python3
"""Local stdio MCP: compact discovery + bounded execution against private IPC."""
import json, math, os, re, selectors, stat, subprocess, sys, time, uuid
from pathlib import Path
from catalog import OPS, TOOLS
ROOT=Path(__file__).resolve().parent
MAX_MESSAGE=131072

def validate(value,schema):
    kind=schema.get('type')
    if kind=='object':
        if not isinstance(value,dict):raise ValueError('Expected object')
        if any(k not in value for k in schema.get('required',[])):raise ValueError('Missing required field')
        if len(value)>schema.get('maxProperties',10000):raise ValueError('Too many properties')
        for key,item in value.items():
            if key in schema.get('properties',{}):validate(item,schema['properties'][key])
            elif schema.get('additionalProperties') is False:raise ValueError('Unknown field: '+key)
            elif isinstance(schema.get('additionalProperties'),dict):validate(item,schema['additionalProperties'])
    elif kind=='array':
        if not isinstance(value,list) or len(value)>schema.get('maxItems',10000):raise ValueError('Invalid array')
        for item in value:validate(item,schema.get('items',{}))
    elif kind in ('integer','number'):
        if isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or kind=='integer' and value!=int(value) or not schema.get('minimum',-1e30)<=value<=schema.get('maximum',1e30):raise ValueError('Invalid number')
    elif kind=='string' and not isinstance(value,str):raise ValueError('Expected string')
    elif kind=='boolean' and not isinstance(value,bool):raise ValueError('Expected boolean')
    if 'enum' in schema and value not in schema['enum']:raise ValueError('Unsupported enum value')

def control_root():
    if sys.platform=='win32':base=Path(os.environ['APPDATA'])
    elif sys.platform=='darwin':base=Path.home()/'Library'
    else:base=Path(os.environ.get('XDG_CONFIG_HOME',str(Path.home()/'.config')))
    return base/'FieldStudio'/'control'

def descriptors():
    root=control_root()
    if root.is_symlink():raise ValueError('Control root is a symlink')
    results=[]
    if not root.exists():return results
    for directory in root.iterdir():
        if not re.fullmatch('[0-9a-f]{32}',directory.name) or directory.is_symlink() or not directory.is_dir():continue
        if os.name!='nt':
            info=directory.stat()
            if info.st_uid!=os.getuid() or stat.S_IMODE(info.st_mode)&0o077:continue
        file=directory/'instance.json'
        if file.is_symlink():continue
        try:
            if file.stat().st_size>4096:continue
            data=json.loads(file.read_text())
            if data.get('instance_id')==directory.name and 0<=time.time()*1000-data['heartbeat_ms']<15000:results.append((directory,data))
        except (OSError,ValueError,KeyError,TypeError):continue
    return results

def call(operation,args,instance_id,deadline):
    if operation not in OPS:raise ValueError('Unknown operation; search first')
    validate(args,OPS[operation][1])
    if operation=='list_instances':return [{'instance_id':d['instance_id'],'protocol':d['protocol']} for _,d in descriptors()]
    if not instance_id or not re.fullmatch('[0-9a-f]{32}',instance_id):raise ValueError('Select an explicit instance_id with list_instances')
    match=next(((folder,data) for folder,data in descriptors() if data['instance_id']==instance_id),None)
    if not match:raise ValueError('Instance unavailable; enable COM > local AI / MCP control')
    folder,data=match;nonce=uuid.uuid4().hex;request=folder/f'request-{nonce}.json';response=folder/f'response-{nonce}.json';temporary=folder/f'.pending-{nonce}'
    limit=min(deadline,time.monotonic()+5)
    payload={'token':data['token'],'operation':operation,'args':args,'expires_ms':time.time()*1000+max(1,(limit-time.monotonic())*1000)}
    encoded=json.dumps(payload,allow_nan=False).encode()
    if len(encoded)>65536:raise ValueError('Control arguments exceed 64 KiB')
    try:
        fd=os.open(temporary,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
        with os.fdopen(fd,'wb') as stream:stream.write(encoded)
        os.replace(temporary,request)
        while time.monotonic()<limit:
            if response.exists():
                if response.is_symlink() or response.stat().st_size>262144:raise ValueError('Invalid response')
                result=json.loads(response.read_text())
                if not result['ok']:raise ValueError(result.get('error','Control request failed'))
                return result['result']
            time.sleep(.01)
        raise TimeoutError('App did not respond; a started operation may have completed. Inspect state before retrying.')
    finally:
        for file in (temporary,request,response):
            try:file.unlink()
            except FileNotFoundError:pass

def search(args):
    validate(args,TOOLS[0]['inputSchema']);query=args['query'].strip().lower()
    if not query or len(query)>200:raise ValueError('Search query must be 1–200 characters')
    words=set(re.findall(r'[a-z0-9]+',query));ranked=[]
    if words&{'performance','latency','cpu','dropout','dropouts','crackle'}:words.add('state')
    if words&{'volume','tempo','sync','clock','metronome','synth','effect','adsr','automation','mixer'}:words.add('parameters')
    if words&{'arpeggio','endless','pattern','finger','hold','chord'}:words.update(('sequence','parameters'))
    for name,(description,schema) in OPS.items():
        score=(100 if query==name else 0)+sum(5 if word in name else 1 if word in description.lower() else 0 for word in words)
        if score:ranked.append((score,name,description,schema))
    ranked.sort(key=lambda x:(-x[0],x[1]))
    return {'operations':[{'name':n,'description':d,'inputSchema':s,'call':f'await fieldstudio.{n}(args)'} for _,n,d,s in ranked[:args.get('limit',4)]],'code_runtime':'Linux QuickJS-NG + seccomp; install requirements-code-mode.txt. Portable operations plans work on all platforms.'}

def execute(args):
    validate(args,TOOLS[1]['inputSchema'])
    if ('code' in args)==('operations' in args):raise ValueError('Supply exactly one of code or operations')
    deadline=time.monotonic()+15;trace=[];maximum=args.get('max_calls',32);limit=args.get('max_output_chars',4000)
    def invoke(name,values):
        if len(trace)>=maximum or time.monotonic()>=deadline:raise ValueError('Execution budget exceeded')
        entry={'operation':name,'status':'started'};trace.append(entry)
        try:
            result=call(name,values,args.get('instance_id'),deadline);entry['status']='complete';return result
        except Exception as error:entry.update(status='failed',error=str(error)[:200]);raise
    result=None;failure=None
    try:
        if 'operations' in args:
            # Validate the entire portable plan before its first side effect.
            for item in args['operations']:
                if item['operation'] not in OPS:raise ValueError('Unknown operation')
                validate(item.get('args',{}),OPS[item['operation']][1])
            result=[]
            for item in args['operations']:
                value=invoke(item['operation'],item.get('args',{}))
                if 'select' in item:
                    if not isinstance(value,dict):raise ValueError('select requires an object result')
                    value={key:value[key] for key in item['select'] if key in value}
                result.append(value)
        else:result=javascript(args,invoke,deadline)
    except Exception as error:failure=str(error)[:1500]
    encoded=json.dumps(result,allow_nan=False,ensure_ascii=True)
    if len(encoded)>limit:result={'truncated':True,'characters':len(encoded),'hint':'Project/filter results inside code or use portable select.'}
    return {'result':result,'error':failure,'calls':trace,'non_atomic':True}

def javascript(args,invoke,deadline):
    if sys.platform!='linux':raise ValueError('Sandboxed JavaScript requires Linux; use operations plans on this platform')
    code=args['code']
    if not code.strip() or len(code.encode())>16384:raise ValueError('Code must be 1–16384 bytes')
    probe=subprocess.run([sys.executable,'-I',str(ROOT/'sandbox.py')],input='{"probe":true}\n',text=True,capture_output=True,timeout=3)
    try:data=json.loads(probe.stdout)
    except ValueError:raise ValueError('Install requirements-code-mode.txt and libseccomp2; sandbox unavailable')
    if data.get('sandbox')!='seccomp' or data.get('file_open')!=-1 or data.get('socket')!=-1:raise ValueError('Sandbox probe failed')
    process=subprocess.Popen([sys.executable,'-I',str(ROOT/'sandbox.py')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,env={'PATH':'/usr/bin:/bin','LANG':'C.UTF-8'},start_new_session=True,bufsize=0)
    selector=selectors.DefaultSelector();selector.register(process.stdout,selectors.EVENT_READ);buffer=b''
    def send(value):
        data=memoryview((json.dumps(value,allow_nan=False)+'\n').encode())
        while data:
            written=process.stdin.write(data)
            if not written:raise ValueError('Sandbox pipe closed')
            data=data[written:]
    try:
        send({'operations':list(OPS),'input':args.get('input',{}),'code':code,'function':bool(re.match(r'^(?:async\s+)?(?:function\b|(?:\([^)]*\)|[A-Za-z_$][\w$]*)\s*=>)',code.lstrip()))})
        while time.monotonic()<deadline:
            if not selector.select(.05):
                if process.poll() is not None:raise ValueError('Sandbox exited before completion')
                continue
            chunk=os.read(process.stdout.fileno(),65536)
            if not chunk:raise ValueError('Sandbox closed unexpectedly')
            buffer+=chunk
            if len(buffer)>2*1024*1024:raise ValueError('Sandbox output limit exceeded')
            while b'\n' in buffer:
                line,buffer=buffer.split(b'\n',1);message=json.loads(line)
                if message['type']=='ready':continue
                if message['type']=='error':raise ValueError(message['error'])
                if message['type']=='done':
                    if message.get('failure'):raise ValueError(message['failure']['message'])
                    return message.get('result')
                if message['type']!='calls' or not isinstance(message.get('calls'),list) or not 1<=len(message['calls'])<=32:raise ValueError('Invalid sandbox protocol')
                responses=[]
                for request in message['calls']:
                    try:responses.append({'id':request['id'],'ok':True,'value':invoke(request['name'],request.get('args',{}))})
                    except Exception as error:responses.append({'id':request['id'],'ok':False,'error':str(error)})
                send({'type':'results','results':responses})
        raise TimeoutError('JavaScript execution deadline exceeded')
    finally:
        selector.close();process.kill();process.wait();process.stdin.close();process.stdout.close()

def handle(message):
    method=message.get('method');params=message.get('params',{})
    if method=='initialize':return {'protocolVersion':'2024-11-05','capabilities':{'tools':{}},'serverInfo':{'name':'opvyn','version':'0.1.0'}}
    if method=='ping':return {}
    if method=='tools/list':return {'tools':TOOLS}
    if method=='tools/call':
        try:
            name=params.get('name');value=search(params.get('arguments',{})) if name=='search' else execute(params.get('arguments',{})) if name=='execute' else (_ for _ in ()).throw(ValueError('Unknown tool'))
            return {'content':[{'type':'text','text':json.dumps(value,allow_nan=False)}],'isError':bool(isinstance(value,dict) and value.get('error'))}
        except Exception as error:return {'content':[{'type':'text','text':str(error)[:1500]}],'isError':True}
    raise ValueError('Unsupported MCP method')

def main():
    while True:
        line=sys.stdin.buffer.readline(MAX_MESSAGE+1)
        if not line:return
        message={}
        try:
            if len(line)>MAX_MESSAGE:raise ValueError('MCP message too large')
            message=json.loads(line)
            if 'id' not in message:continue
            result=handle(message);response={'jsonrpc':'2.0','id':message['id'],'result':result}
        except Exception as error:response={'jsonrpc':'2.0','id':message.get('id') if 'message' in locals() else None,'error':{'code':-32602,'message':str(error)[:1500]}}
        sys.stdout.write(json.dumps(response,allow_nan=False)+'\n');sys.stdout.flush()
if __name__=='__main__':main()
