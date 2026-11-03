---
name: control
description: Control an enabled local OPVYN instance: inspect parameters, compose step sequences, audition MIDI notes and manage playback using compact MCP Code Mode.
---
Use search to discover exact operations and schemas. Use execute to call list_instances first, then pin instance_id for all app operations. Never guess parameter IDs or enum indices: query list_parameters with a narrow query. Enable COM > local AI / MCP control in the desired instance.

On Linux, JavaScript uses await fieldstudio.operation(args); return only needed fields. Install requirements-code-mode.txt and libseccomp2; execution fails closed without the sandbox. Do not request direct filesystem, shell or network access. Other platforms can use a validated operations array through the same execute tool.

Edits across calls are not atomic. Inspect the returned trace and get_state before retrying any failed/timeout script. Never automatically replay it. Keep note auditions bounded; send note-off or transport panic afterward, especially with Hold/latch. Do not claim hardware equivalence, exact sonic parity or verified physical audio latency. No record, file, destructive tape or project-loading operations are exposed.
