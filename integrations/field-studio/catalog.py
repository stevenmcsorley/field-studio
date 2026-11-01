"""Discoverable API schemas; never sent wholesale as MCP tool definitions."""
def schema(properties=None, required=()):
    return {'type':'object','properties':properties or {},'required':list(required),'additionalProperties':False}
def number(lo,hi,integer=False):return {'type':'integer' if integer else 'number','minimum':lo,'maximum':hi}
OPS={
 'list_instances':('List enabled, responsive local Field Studio instances; choose an instance_id.',schema()),
 'get_state':('Read project, tempo, transport and sequencer state.',schema()),
 'list_parameters':('Discover parameter IDs, values, ranges and enum choices. Search before setting values.',schema({'query':{'type':'string'},'limit':number(1,64,True)})),
 'set_parameters':('Set up to 64 discovered parameters in their actual units. Validates all values before edits.',schema({'values':{'type':'object','additionalProperties':{'type':'number'},'maxProperties':64}},('values',))),
 'get_sequence':('Read paginated notes/velocities; omit bank for Endless/Pattern or choose Finger bank 0–15.',schema({'start':number(0,998,True),'count':number(1,128,True),'bank':number(0,15,True)})),
 'set_sequence':('Edit bounded step notes (-1 rest) and optional length; choose bank for Finger. No audio files affected.',schema({'steps':{'type':'array','maxItems':999,'items':schema({'index':number(0,998,True),'note':number(-1,127,True),'notes':{'type':'array','maxItems':8,'items':number(0,127,True)},'velocity':number(0,1)},('index',))},'length':number(1,999,True),'bank':number(0,15,True)},('steps',))),
 'transport':('Play/stop/rewind tape or panic (clear held notes). Recording and destructive edits are not exposed.',schema({'action':{'type':'string','enum':['play','stop','rewind','panic']}},('action',))),
 'notes':('Press/release up to 16 MIDI notes to audition or drive sequencers. Latches may need transport panic.',schema({'notes':{'type':'array','maxItems':16,'items':number(0,127,True)},'velocity':number(0,1),'on':{'type':'boolean'}},('notes','on'))),
 'manual_step':('Advance Endless once when manual mode is enabled.',schema()),
}
TOOLS=[
 {'name':'search','description':'Discover Field Studio operations and exact schemas by keyword. No app edits. Search before execute; do not guess parameter IDs. Only matched schemas enter context.','inputSchema':schema({'query':{'type':'string'},'limit':number(1,9,True)},('query',))},
 {'name':'execute','description':'Control an explicitly selected local Field Studio instance. Supply JavaScript async body using await fieldstudio.operation(args), return only needed fields; Linux QuickJS-NG+seccomp required for code. Or use a portable bounded operations array on any OS. No direct JS filesystem/network/process access. Runs are non-atomic; earlier edits persist on error, never auto-replay. Trace returned. Max 32 calls, 15 seconds, bounded output.','inputSchema':schema({'instance_id':{'type':'string'},'code':{'type':'string'},'input':{'type':'object'},'operations':{'type':'array','maxItems':32,'items':schema({'operation':{'type':'string'},'args':{'type':'object'},'select':{'type':'array','items':{'type':'string'}}},('operation',))},'max_calls':number(1,32,True),'max_output_chars':number(256,16000,True)},())}
]
