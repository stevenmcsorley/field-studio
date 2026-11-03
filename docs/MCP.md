# Local AI control and Code Mode

OPVYN exposes two MCP tools: **search** and **execute**. The [Cloudflare Code Mode approach](https://blog.cloudflare.com/code-mode-mcp/) inspired the compact discovery/execution design. This implementation runs locally; it does not call Cloudflare or require a Workers account.

`search` returns only relevant operation schemas. `execute` can chain app calls in bounded JavaScript and return a small projection, avoiding repeated tool descriptions and full intermediate results in the model context. The initial API has nine discoverable operations. Its compact JSON tool declarations measured 1,384 bytes versus 3,007 bytes for nine flat declarations during development; this is a schema-byte comparison, not a billed-token or cost guarantee. Actual savings depend on the model, scripts, searches and returned data.

## Enable an instance

1. Run the standalone, or open a plugin instance in the DAW.
2. Choose **COM → Enable local AI / MCP control** for that instance. It is off by default, is not saved with projects and must be enabled again in a new instance.
3. Configure your MCP client to launch `integrations/field-studio/server.py` with Python 3.10 or newer. That directory contains a portable local plugin manifest, `mcp.json`, a control skill and the server. It is included in the binary archives as well as source. Use an absolute interpreter/script path when configuring a client directly.
4. Ask the agent to search for `list_instances`, execute it, and pin the returned `instance_id`. It must select an explicit instance; it never silently chooses the first plugin.

A generic stdio client configuration (replace the paths):

```json
{
  "mcpServers": {
    "opvyn": {
      "command": "/absolute/path/to/python",
      "args": ["/absolute/path/to/integrations/field-studio/server.py"]
    }
  }
}
```

This ships the integration; it does not silently edit or register your AI client's global configuration. The portable plugin's Python command may need changing to your venv interpreter or platform's Python executable.

## JavaScript Code Mode — Linux

Create a Python venv and install the pinned runtime:

```sh
python3 -m venv .venv-mcp
.venv-mcp/bin/python -m pip install -r integrations/field-studio/requirements-code-mode.txt
```

Use that venv interpreter as the MCP command. Linux also requires `libseccomp2` (Ubuntu package). Execution probes the sandbox and fails closed if QuickJS-NG or seccomp is unavailable. The untrusted child has no direct JS filesystem, network, process, environment, modules or timer API; OS-level seccomp denies file opens and sockets, and CPU/memory/deadline limits apply. The trusted parent separately validates every app call. This reuses/adapts the earlier local Frameforge sandbox approach; it is not a Cloudflare Worker isolate.

After operation/parameter discovery, an example `execute` call is:

```json
{
  "instance_id": "COPY_THE_DISCOVERED_ID",
  "code": "await fieldstudio.set_parameters({values:{sequencer:5,sequenceOn:1,tempo:96,seqDivision:4}}); const chords=[[60,64,67],[57,60,64],[65,69,72],[67,71,74]]; await fieldstudio.set_sequence({length:16,steps:Array.from({length:16},(_,i)=>({index:i,notes:i%4===0?chords[i/4]:[]}))}); const s=await fieldstudio.get_sequence({start:0,count:16}); return {length:s.length,notes:s.steps.map(x=>x.notes)};"
}
```

Parameter IDs/enum indices should be discovered rather than guessed; the snippet illustrates the current schema. Select Pattern in the app to view it. To audition, send `notes` with `[60]`, `on:true`; release afterward or call `transport` with `action:"panic"`, especially if Hold/latch is enabled. The audio device/DAW must be processing audio for queued notes to sound. AI calls do not supply sample-accurate note scheduling; the sequencer engine provides musical timing.

Scripts use an async function body (`await fieldstudio.operation(args)`), may loop/filter returned JSON, and should return only needed fields. An async arrow function is also accepted. Limits: 15 seconds total, 32 app calls, 16 KiB script, 32 MiB JS heap, bounded bridge messages and up to 16,000 characters of returned result. The call trace is returned separately. Infinite loops and direct file/network probes are covered by tests.

## Portable plans — Windows, macOS and Linux

The same `execute` tool accepts a bounded declarative operations array with Python's standard library only. Arbitrary JavaScript is currently Linux-only; portable plans do not pretend to provide the same runtime on other OSes.

```json
{
  "instance_id": "COPY_THE_DISCOVERED_ID",
  "operations": [
    {"operation":"set_parameters","args":{"values":{"tempo":110,"volume":0.6}}},
    {"operation":"get_state","args":{},"select":["project","tempo","length"]}
  ]
}
```

Exactly one of `code` or `operations` is accepted. Search and discovery do not need a JS runtime. The server does not bundle a language model or charge a hosting fee; your AI client's/model provider's normal charges still apply.

## API and scope

| Operation | Capability |
|---|---|
| list_instances | Discover enabled, recently responsive instances without returning their secret token |
| get_state | Read project, transport, step/length, tempo and voice/held-note counts |
| list_parameters | Query actual parameter IDs, ranges, values and enum labels |
| set_parameters | Validate a parameter batch before applying it |
| get_sequence | Read a bounded step page, chord notes and velocities |
| set_sequence | Write notes/chords/rests and length to the shared Pattern/Endless buffer or a Finger bank |
| transport | Play, stop, rewind tape or panic |
| notes | Queue up to 16 MIDI notes through the existing GUI/audio handoff |
| manual_step | Advance manual Endless once |

No recording, filesystem, project-load/save, sample import, destructive tape edits or arbitrary app code is exposed in this first API. User messages/prompts cannot turn a Code Mode script into shell access. Instance control is engine-focused; it does not navigate every GUI page.

Commands use a private per-user, per-instance file queue, processed on the app's message thread, never by the audio callback. There is no listening TCP port. Each request has a capability token, random ID, bounded size and expiry; request IDs are not automatically replayed. POSIX directories are owner-only; Windows relies on the user profile's access controls. No credentials appear in discovery responses or the model context. Files live under `FieldStudio/control` in the user application-data directory, and the active instance removes its directory when disabled/closed. Stale instances are ignored. Disable control through COM to stop accepting commands.

**Runs are not atomic.** Earlier successful calls remain applied after a later failure or timeout. Inspect the returned trace and app state before retrying; never auto-replay a failed script. A timed-out native request that already started may have completed. Notes/held latches require note-off or panic before disconnecting if you want silence.

Validation includes stdio MCP startup/discovery, live native-engine parameter/chord/note operations, batch validation, invalid authorization, JS projections, partial failures, output bounds and infinite-loop interruption. CI runs portable/native tests on all OSes and seccomp Code Mode tests on Linux. This is not a guarantee of zero audio interruption during command/UI lock contention.

## Tempo and sync parameters

Search `tempo`, `sync`, `clock` or `metronome` to discover parameter operations. `list_parameters` exposes `tempo`, `midiClockMode`, `tempoTapeLink`, `midiClockRelay`, `metronomeLevel` and `metronomeSound`. `get_state` includes effective tempo, actual tape speed, clock lock, mode, relay and tape-link values. Choose routing in the standalone/DAW first; MCP does not enumerate or connect hardware ports. See [tempo setup](TEMPO_SYNC.md).

## Beta 10 timing semantics

`get_state` reads the latest published telemetry (approximately 30 Hz while audio runs), without acquiring the live engine guard. Project/source labels in that snapshot are bounded UTF-8 strings. Parameter values may be newer than the effective render telemetry. Transport and `manual_step` return `queued: true`; this acknowledges admission, not completion. `manual_step` no longer returns a synchronously applied step. Read a later state to observe the result; batching a state query immediately after a queued command is not a completion barrier. Stop/panic remain admissible with a full ordinary control queue. The tool set stays `search` / `execute`; sequence bulk operations still use the engine guard.

OPVYN retains `integrations/field-studio`, the `fieldstudio` JavaScript object, existing endpoint schemas and the `FieldStudio/control` discovery directory for compatibility. Existing MCP client configurations continue to work; new manifests use the `opvyn` display/server name. See [rename notes](RENAMING.md).
