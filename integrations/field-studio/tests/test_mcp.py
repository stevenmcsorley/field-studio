import importlib.util,json,os,subprocess,sys,tempfile,time,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
import server

class Discovery(unittest.TestCase):
    def test_two_tools(self):
        self.assertEqual([t['name'] for t in server.handle({'method':'tools/list'})['tools']],['search','execute'])
        self.assertIn('set_parameters',[o['name'] for o in server.search({'query':'volume'})['operations']])
        self.assertIn('set_sequence',[o['name'] for o in server.search({'query':'set_sequence'})['operations']])
    def test_strict_and_limits(self):
        for args in ({'operations':[],'code':'return 1'},{'operations':[{'operation':'shell','args':{}}]},{'code':'','max_calls':33}):
            try:r=server.execute(args)
            except ValueError:continue
            self.assertIsNotNone(r['error'])
    def test_stdio_protocol(self):
        requests=[{'jsonrpc':'2.0','id':1,'method':'initialize','params':{}},{'jsonrpc':'2.0','id':2,'method':'tools/list'},{'jsonrpc':'2.0','id':3,'method':'tools/call','params':{'name':'search','arguments':{'query':'sequence'}}}]
        result=subprocess.run([sys.executable,str(ROOT/'server.py')],input='\n'.join(map(json.dumps,requests))+'\n',text=True,capture_output=True,timeout=10,check=True)
        rows=[json.loads(line) for line in result.stdout.splitlines()];self.assertEqual([r['id'] for r in rows],[1,2,3]);self.assertEqual(len(rows[1]['result']['tools']),2);self.assertFalse(rows[2]['result']['isError'])

class LiveControl(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        binary=Path(os.environ.get('FIELD_CONTROL_TEST_BINARY','build/field_plugin_tests_artefacts/Release/field_plugin_tests'+('.exe' if sys.platform=='win32' else ''))).resolve()
        if not binary.is_file():raise RuntimeError('Build native field_plugin_tests before MCP integration tests')
        cls.temp=tempfile.TemporaryDirectory();cls.marker=Path(cls.temp.name)/'ready';cls.process=subprocess.Popen([str(binary),'--control-bridge-session',str(cls.marker)],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        deadline=time.monotonic()+15
        while not cls.marker.exists():
            if cls.process.poll() is not None:raise RuntimeError('Native control fixture failed to start')
            if time.monotonic()>deadline:raise RuntimeError('Native control fixture timed out')
            time.sleep(.02)
        cls.endpoint=Path(cls.marker.read_text());cls.instance_id=json.loads(cls.endpoint.read_text())['instance_id']
    @classmethod
    def tearDownClass(cls):
        cls.marker.with_name('ready.stop').touch()
        try:cls.process.wait(timeout=5)
        except subprocess.TimeoutExpired:cls.process.kill();cls.process.wait()
        cls.temp.cleanup()
    def operation(self,operation,args=None):
        result=server.execute({'instance_id':self.instance_id,'operations':[{'operation':operation,'args':args or {}}]})
        self.assertIsNone(result['error'],result);return result['result'][0]
    def test_a_discovery(self):
        self.assertIn(self.instance_id,[row['instance_id'] for row in self.operation('list_instances')]);self.assertEqual(self.operation('get_state')['project'],'Untitled')
    def test_b_parameter_and_validation(self):
        self.operation('set_parameters',{'values':{'volume':.37}})
        result=server.execute({'instance_id':self.instance_id,'operations':[{'operation':'set_parameters','args':{'values':{'volume':.1,'not_a_parameter':1}}}]})
        self.assertIsNotNone(result['error']);self.assertAlmostEqual(next(p['value'] for p in self.operation('list_parameters',{'query':'volume'}) if p['id']=='volume'),.37,places=4)
    def test_c_chords(self):
        self.operation('set_sequence',{'length':4,'steps':[{'index':0,'notes':[60,64,67],'velocity':.7},{'index':1,'note':-1}]})
        state=self.operation('get_sequence',{'count':2});self.assertEqual(state['length'],4);self.assertEqual(state['steps'][0]['notes'],[60,64,67]);self.assertEqual(state['steps'][1]['note'],-1)
        result=server.execute({'instance_id':self.instance_id,'operations':[{'operation':'set_sequence','args':{'bank':0,'steps':[{'index':0,'note':61},{'index':32,'note':62}]}}]});self.assertIsNotNone(result['error'])
    def test_d_notes(self):
        self.operation('set_parameters',{'values':{'sequencer':5,'sequenceOn':1,'seqHold':0}});self.assertEqual(self.operation('notes',{'notes':[60],'on':True})['queued'],1);time.sleep(.03);self.operation('notes',{'notes':[60],'on':False});self.operation('transport',{'action':'panic'})
    def test_e_reject_auth(self):
        nonce='12345678123456781234567812345678';folder=self.endpoint.parent;request=folder/f'request-{nonce}.json';reply=folder/f'response-{nonce}.json'
        request.write_text(json.dumps({'token':'wrong','expires_ms':time.time()*1000+3000,'operation':'set_parameters','args':{'values':{'volume':.2}}}))
        deadline=time.monotonic()+3
        while not reply.exists() and time.monotonic()<deadline:time.sleep(.01)
        self.assertFalse(json.loads(reply.read_text())['ok']);reply.unlink()
    @unittest.skipUnless(sys.platform=='linux' and importlib.util.find_spec('quickjs'),'Linux Code Mode dependency required')
    def test_f_code_mode(self):
        result=server.execute({'instance_id':self.instance_id,'code':'await fieldstudio.set_parameters({values:{volume:0.42}}); const p = await fieldstudio.list_parameters({query:"volume"}); return p.find(p=>p.id==="volume").value;'})
        self.assertIsNone(result['error'],result);self.assertAlmostEqual(result['result'],.42,places=4)
        result=server.execute({'instance_id':self.instance_id,'code':'await fieldstudio.set_parameters({values:{volume:0.43}}); throw new Error("intentional partial run");'})
        self.assertIn('intentional',result['error']);self.assertEqual(result['calls'][0]['status'],'complete')
        result=server.execute({'instance_id':self.instance_id,'code':'return {filesystem:typeof require, process:typeof process, fetch:typeof fetch};'})
        self.assertEqual(set(result['result'].values()),{'undefined'})
        result=server.execute({'instance_id':self.instance_id,'code':'while(true){}'})
        self.assertIsNotNone(result['error'])
        result=server.execute({'instance_id':self.instance_id,'code':'return "x".repeat(20000)','max_output_chars':256})
        self.assertTrue(result['result']['truncated'])

if __name__=='__main__':unittest.main()
