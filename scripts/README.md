# How the Scripts Work

What each script does, how they fit together, and why they are built this way.
For running them, see [set-up-and-test.md](../docs/how-to/set-up-and-test.md).

[← README](../README.md)

## The layout

```
scripts/
  e2e.sh               runs every step in order
  steps/               one script per job
  helpers/             the functions the steps are built from
    settings.sh        loads the settings; every script starts with it
  build-config.sh      which steps e2e.sh runs
  config.sh            the values you choose (your own copy, not in the repo)
  windows/             everything for Windows
```

- **Everything is run from `scripts/`.** One run location keeps every path the same.
- **A step script is a list of function calls.** The functions live in `helpers/`, grouped by subject, and take everything they need as arguments. They hold no project values, and they print their own messages.
- **Nothing is typed in.** The scripts find the VM's address and the PC's LAN address themselves.

## Where the settings come from

Every script begins with `source helpers/settings.sh`. That file builds the settings in three layers:

```
 1. config.sh            the values someone chose
        ▼
 2. settings.sh          the values that are the same for every setup
        ▼
 3. SYSTEM_SETTINGS      another system's values, if a start script names a file
```

| Layer | Holds | Why it is separate |
|-------|-------|--------------------|
| `config.sh` | Project name, VM name, port, Wi-Fi name and password | The only values a person has to decide; it holds a password, so it is not in the repo |
| `helpers/settings.sh` | SSH options, the Multipass and PlatformIO commands, which network functions to load | They never change, so nobody should have to copy or fill them in |
| `SYSTEM_SETTINGS` | Values that replace layer 2 on another system | The Linux scripts never ask which system they are on; a start script hands them a file instead |

## Standing alone, or for a project

The bridge reads three config files. Standing alone it uses its own. A project that uses the bridge names its own three, as full paths, in environment variables:

| File | Variable | Standing alone | Holds |
|------|----------|----------------|-------|
| Script config | `SCRIPT_CONFIG` | `scripts/config.sh` | Project name, VM name, port, Wi-Fi |
| Firmware config | `FIRMWARE_CONFIG` | `include/config.h` | Routing table, downlink topics, line logging |
| Build config | `BUILD_CONFIG` | `scripts/build-config.sh` | The steps `e2e.sh` runs |

A project keeps these files in its own repository, wherever suits it, and sets the three variables when it runs `e2e.sh` from this folder. The dummy data source is a setting in the bridge's own `include/config.h` only, so a project's build has none.

**Why environment variables:** `e2e.sh` runs each step as a separate command, and an environment variable reaches all of them without being passed along by hand.

**Why the project's files are read where they are:** nothing of a project's is copied into this repository, so the bridge can be updated or cloned afresh without losing anything.

## What `e2e.sh` does

`e2e.sh` takes no options. `build-config.sh` holds the list of steps, each with a switch in front of it, and `e2e.sh` loops over that list in order: a step runs when its switch is `1`.

```
STEPS=(
    "0 steps/nuke.sh"                        delete everything the scripts created
    "0 steps/clean-build.sh"                 delete the firmware's build output
    "1 steps/create-vm.sh"                   create the broker's VM, if there is none
    "1 steps/setup-certs.sh"                 certificates, the firmware's files
    "1 steps/set-port-rule.sh"               port rule, connection test
    "1 steps/build-and-upload.sh"            the firmware
    "1 steps/test-bridge.sh"                 the bridge's own test
)
KEEP_PORT_RULE=0                             1 leaves the port rule in place at the end
```

- **Switches in a file, not options on the command.** What a run does is set once and read at a glance; the command is always the same.
- **One list, one loop.** The list is the steps, their order and their switches in one place, so `e2e.sh` holds no condition per step. Adding a step is one line in the list.
- **The committed values suit a first run:** everything on, nothing deleted. They also suit every later run, because `create-vm.sh` does nothing when the VM is already there. Changing a switch for one run is a local edit that is not committed.
- **A project's build config has its own list**, so its choices do not touch this file.
- **A project's list has no `test-bridge.sh`.** That test is the bridge's own: it needs the dummy data source and a jumper from TX to RX, neither of which a project has. A project tests its own link.

## The steps

| Script | What it does | Why |
|--------|--------------|-----|
| `nuke.sh` | Deletes the VM, the certificates, the SSH key, the firmware's generated files and the port rule. Asks twice first | Gives a clean slate to prove the setup from |
| `clean-build.sh` | Deletes the firmware's build output (`.pio/build`) | The build reuses its earlier setup and does not notice when the list of modules changes. Switch this on after a pull or a branch switch that adds or removes a module, or when a build stops at a header that is "not found". It is off as committed, because a build from scratch takes minutes |
| `create-vm.sh` | Creates the Multipass VM and installs Mosquitto, set for TLS with client certificates | The broker runs in a VM so the PC itself is not changed |
| `setup-certs.sh` | Makes the SSH key, CA and client certificate if they are missing; remakes the server certificate; writes the firmware's files; installs the broker's certificates | The server certificate depends on an address that changes, so it is remade every run. The CA and client certificate are kept, so the firmware's certificates stay valid |
| `set-port-rule.sh` | Looks up the PC's and the VM's addresses, sets the port rule, tests the TLS connection | The rule is a step of its own so that it can be put back, after a restart of the PC, without redoing the certificates. The test is here because this is the first moment the broker can be reached |
| `remove-port-rule.sh` | Removes the port rule | Closes the PC again by hand. It is not in the list of steps: `e2e.sh` removes the rule itself when it ends |
| `build-and-upload.sh` | Points the firmware at its config, then builds and uploads it | See below |
| `test-bridge.sh` | Waits for a line on the uplink topic, then publishes a message down and waits for it to come back up | A message on a topic is a clear pass or fail, and proves the whole chain at once |

## The report

A run prints a great deal. When it ends, `e2e.sh` prints a short report: one line for each step that was switched on, in order, with how it went.

```
---------------- Report ----------------
 1. create-vm                          success
 2. setup-certs                        success
 3. set-port-rule                      success
 4. build-and-upload                   success
 5. test: a line comes up              success
 6. test: a message goes down          success
----------------------------------------
```

- **It is printed whether the run finished or failed**, and it is the last thing on the screen.
- **A step that fails is marked `FAILED`**, and the steps after it `not run`. A step that is switched off is left out.
- **A step can report its own checks.** `test-bridge.sh` adds a line for each of its two, and then gets no line of its own.
- **One report for a whole run.** When a project's build runs `e2e.sh`, the bridge's steps go into the project's report, marked `bridge:`, and the project prints it. The script that starts a run starts the report and hands its file on in `REPORT_FILE`.
- **A step run by itself reports nothing**, because no report was started.

The functions are in `helpers/report.sh`. A project's own scripts load that file to add their steps and checks to the same report.

## Testing through the broker

`helpers/bridge-test.sh` holds two checks that work for any topics and any lines:

| Function | What it does |
|----------|--------------|
| `check_line_arrives` | Waits for one line on a topic |
| `check_exchange` | Publishes a message to one topic and checks that an expected line arrives on another |

The bridge's own test, `test-bridge.sh`, is built from them: a dummy line has to arrive, and a message sent down has to come back up unchanged. A project that uses the bridge loads the same file and calls the two checks with its own topics and lines, to test its own device.

**Why they return and do not stop the script:** the caller knows what is being tested, so it adds the hint that helps (check the jumper, check the wiring) and stops the script itself.

## How the firmware gets its config

The firmware does not include `config.h` directly. It includes one generated file:

```
include/generated/config_in_use.h      written by build-and-upload.sh, not in the repo
    #define BUILT_FOR_PROJECT <1 or 0>
    #include "<full path of the config.h for this build>"
```

`build-and-upload.sh` writes that file before every build, pointing at the bridge's own `include/config.h` or at the project's.

`BUILT_FOR_PROJECT` is `1` when a project named its own firmware config. It is how the firmware knows to include the handshake with the MCU: a project building the bridge is what makes an MCU expected, so there is no switch to set. Built standing alone it is `0`, and the handshake is left out of the firmware.

**Why a generated pointer:** the compiler has to be told which file to use. A switch in the build system can go stale without a sign, with the build quietly keeping the previous config. A file that is rewritten on every run cannot: its contents say which config the last build used, and changing it makes the affected files rebuild.

**One consequence:** a build started from the editor, not from the script, uses whichever config the last scripted build pointed at.

The files in `include/secrets/` work the same way: `setup-certs.sh` writes the Wi-Fi details, the broker's address and the certificates there on every run, so nothing in that folder is edited by hand.

## The port rule

The broker's VM is on a virtual network that the bridge cannot reach over Wi-Fi. The PC passes connections that arrive on its own LAN address, on the broker's port, to the VM.

- It is set by `set-port-rule.sh`, on the LAN address only.
- It is removed when `e2e.sh` ends, so the PC is left closed, unless `KEEP_PORT_RULE` is `1`. A project sets it to `1`, because its device needs the rule after the script ends.
- It does not survive a restart of the PC. Running `bash steps/set-port-rule.sh` puts it back, and `bash steps/remove-port-rule.sh` takes it away.

## Windows

Windows is supported for the bridge standing alone only. A project that uses the bridge builds it from Linux. This layout of the Windows files has not been run on Windows yet.

The scripts need Linux tools, so on Windows they run in WSL. Everything that differs is in `scripts/windows/`, and the Linux scripts hold no Windows checks.

```
windows/
  windows.sh             the start script, run from Git Bash as administrator
  config.sh              the values only a Windows PC needs (your own copy)
  settings.sh            the values that replace the Linux ones
  helpers.sh             the functions windows.sh uses
  network-helpers.sh     the Windows versions of the LAN address and port rule functions
```

`bash windows/windows.sh <script>` does three things around the script it is given:

1. **Checks** the terminal has administrator rights and the three network adapters named in `windows/config.sh` exist. WSL is started first, because its adapter only exists while it runs.
2. **Switches on forwarding** between WSL's network and the VM's, which SSH from WSL to the VM needs, and switches it off again when the run ends.
3. **Runs the script in WSL** with `SYSTEM_SETTINGS=windows/settings.sh`, so layer 3 above replaces the Linux commands with `multipass.exe`, the Windows PlatformIO path and the Windows network functions.

| What differs | On Linux | On Windows |
|--------------|----------|------------|
| Finding the LAN address | `ip` | `netsh`, from the adapter named in `windows/config.sh` |
| The port rule | `iptables` | `netsh`, with a firewall rule added and removed with it |
| `sudo` | Asked for when the port rule changes | Not asked for: the administrator terminal covers it |
| Keys and certificates | In the home folder | In WSL's home folder |

**Why a folder of its own:** a Linux user fills in one short config with no Windows line in it and never opens this folder. Removing it would leave a complete Linux setup.

**See also:** [set-up-and-test.md](../docs/how-to/set-up-and-test.md) · [commands.md](../docs/system/commands.md) · [decision_logs.md](../docs/project-design/decision_logs.md)
