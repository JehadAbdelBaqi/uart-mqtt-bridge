# Using the Bridge from a Project

How a project with its own microcontroller builds the bridge, wires it up and talks to it.

[← README](../../README.md)

## The idea

- **The bridge's repository is not edited.** A project keeps its own config
  files, in its own repository, and hands them to the bridge's scripts.
- **The bridge's code is the same for every project.** What a project decides
  is the link's settings, which letters go to which topics, and what an answer
  looks like.
- **The project's build starts the bridge's build.** One command in the project
  builds and uploads both boards.

```
 your project's repository             the bridge's repository (not edited)
 ─────────────────────────             ────────────────────────────────────
 script config   ─────────┐
 firmware config ─────────┼─ named as full paths ─►  scripts/e2e.sh
 build config    ─────────┘                              │
                                                         ▼
                                              bridge firmware, built with
                                              your config and uploaded
```

## What you need

- **Linux**, with the software in [resources.md](../system/resources.md).
- **The bridge's repository at a release**, cloned beside your project. Use a
  tagged release, so a later change to the bridge cannot break your build.
- **The bridge standing alone working first.** Follow
  [set-up-and-test.md](set-up-and-test.md) once, so that a fault later is known
  to be in the wiring or the config and not in the PC's set-up.

## Wiring

Three wires between the bridge and the microcontroller (MCU):

| Bridge (Genesis Mini) | MCU |
|-----------------------|-----|
| GPIO7 (TX) | its UART RX |
| GPIO6 (RX) | its UART TX |
| GND | GND |

TX goes to RX in both directions. The pins are in `include/board.h`. The
bridge's pins work at 3.3 V, so the MCU's have to as well.

## The three files your project keeps

| File | Named in | Holds | In place of the bridge's |
|------|----------|-------|--------------------------|
| Script config | `SCRIPT_CONFIG` | Project name, VM name, broker port, Wi-Fi network | `scripts/config.sh` |
| Firmware config | `FIRMWARE_CONFIG` | The UART link, the routes and topics, the handshake and the answer, the time limits | `include/config.h` |
| Build config | `BUILD_CONFIG` | Which of the bridge's steps run | `scripts/build-config.sh` |

They can live anywhere in your repository and have any names.

### Script config

Copy the bridge's `scripts/config.example.sh` and fill it in. It holds a Wi-Fi
password, so keep your own copy out of your repository and commit an example
beside it.

```bash
PROJECT_NAME="my-project"      # names the folders the SSH key and certificates are kept in
VM_NAME="broker"               # the Multipass VM that runs the test broker
BROKER_PORT=8883
WIFI_SSID="<your network>"
WIFI_PASSWORD="<your password>"
```

### Firmware config

A header of `#define`s. It has to set everything the bridge's own
`include/config.h` sets, with your values: the bridge holds no fallback, and a
build stops with an error naming any setting that is missing. Every setting is
described in [configuration.md](../system/configuration.md#config-header).

The settings that are yours to decide:

| Setting | What you decide |
|---------|-----------------|
| `UART_BAUD`, `LINE_MAX_LEN`, `LINE_END_CHARACTERS`, `LINE_SKIPPED_CHARACTERS` | The link. Your MCU has to use the same values |
| `UPLINK_ROUTES` | For each first letter your MCU sends, the topic that line is published to |
| `DOWNLINK_TOPICS` | The topics whose messages are written down to your MCU |
| `HANDSHAKE_LETTER` | The letter of your MCU's handshake line |
| `ANSWER_FORMAT`, `ANSWER_FIELD_COUNT`, `FIELD_SEPARATOR`, the `STATUS_TEXT_...` settings | What the bridge's answer looks like in your protocol |
| `BROKER_CONFIRM_LIMIT_MS` | How long the bridge waits for the broker before it answers that a line has not gone through |
| `MCU_QUIET_LIMIT_MS` | How long the bridge hears nothing from your MCU before it counts it as gone |

Two of them depend on your MCU's own timing:

- **`BROKER_CONFIRM_LIMIT_MS` has to be shorter than the time your MCU waits
  for an answer**, so that the bridge's answer always arrives first.
- **`MCU_QUIET_LIMIT_MS` has to be longer than the longest silence your MCU
  keeps** in ordinary running, or the bridge counts it as gone between its
  lines.

If your MCU's firmware keeps these values in headers of its own, the bridge's
config can take them from there, so each is written once.

Leave `DUMMY_LINE_INTERVAL_MS` out: it switches on the bridge's own test lines,
which are for the bridge standing alone.

### Build config

The list of the bridge's steps, each with a switch in front of it. A project
leaves out the two steps that test the bridge standing alone
(`unit-tests.sh` and `test-bridge.sh`), and keeps the port rule in place when
the run ends, because its device needs the broker afterwards:

```bash
STEPS=(
    "0 steps/nuke.sh"            # 1 to start from a clean slate
    "0 steps/clean-build.sh"
    "1 steps/create-vm.sh"
    "1 steps/setup-certs.sh"
    "1 steps/set-port-rule.sh"
    "1 steps/build-and-upload.sh"
)

KEEP_PORT_RULE=1
```

What each step does is in [set-up-and-test.md](set-up-and-test.md).

## Building the bridge with your config

From the bridge's `scripts/` folder, with your three files named as full paths:

```bash
cd <bridge>/scripts
SCRIPT_CONFIG=<path> FIRMWARE_CONFIG=<path> BUILD_CONFIG=<path> bash e2e.sh
```

In practice this line goes in a script of your project, as one step of its own
build, so that one command builds your board and then the bridge.

Naming a firmware config is also what tells the bridge an MCU is expected: the
handshake is then built into its firmware.

The certificates are made by the bridge's scripts, under the project name your
script config gives. Your project holds none.

## What your MCU has to do

| Rule | Why |
|------|-----|
| **Send whole lines**, ended as `LINE_END_CHARACTERS` says, and no longer than `LINE_MAX_LEN` | The bridge cuts the bytes into lines by that rule. A longer line goes up cut short |
| **Start every line with a letter that has a route** | The first letter picks the topic. A line with no route is not published and gets no answer |
| **Shake hands first.** Send a line starting with `HANDSHAKE_LETTER` when it starts | Until a handshake is answered the bridge does not count the MCU as there |
| **Wait for the answer to a line before sending the next** | The bridge holds one line at a time. A second line sent meanwhile takes the first one's place, and the first is never answered |
| **Send something at least once every `MCU_QUIET_LIMIT_MS`**, a handshake if there is nothing else | Silence for that long means the MCU is gone. After that only a handshake brings it back |
| **Read the status of each answer** | It says what became of the line |

**The answer.** With `ANSWER_FORMAT` as `"L,%s,%s"` and `ANSWER_FIELD_COUNT` as
`2`, a line and its answer look like this:

```
 W,5,2350          the MCU's line
 L,W,5,ok          the bridge's answer: the start of the line, then the status
```

| Status | Means | For the MCU |
|--------|-------|-------------|
| `STATUS_TEXT_OK` | The broker has the line. For a handshake: Wi-Fi is up and the broker is connected | Send the next line |
| `STATUS_TEXT_CON_ERR_WIFI` | The bridge has no Wi-Fi. The line was not published | Keep the line and send it again later |
| `STATUS_TEXT_CON_ERR_BROKER` | Wi-Fi is up, the broker is not connected. The line was not published | Keep the line and send it again later |
| `STATUS_TEXT_ERR_BROKER` | The broker did not confirm the line in time | Send it again. It may then arrive twice, so put a number in your lines |

A handshake is answered at once, whatever the bridge can reach, so one exchange
tells the MCU both that the bridge is there and whether its lines can be
published.

**Messages coming down.** Every message on a subscribed topic is written to the
MCU as one line, unchanged. They arrive whenever the broker delivers them,
including while the MCU is waiting for an answer. Whoever publishes to those
topics sends one line per message, within the length limit.

How this works inside the bridge is in
[architecture.md](../system/architecture.md).

## Testing your device through the broker

The bridge's helpers work for any topics and any lines, so your project can
load them for its own tests:

| File | Gives you |
|------|-----------|
| `scripts/helpers/bridge-test.sh` | `check_line_arrives` waits for one line on a topic. `check_exchange` publishes a message to one topic and checks that an expected line arrives on another |
| `scripts/helpers/report.sh` | The report printed at the end of a run. When your build starts the report and then runs the bridge's `e2e.sh`, the bridge's steps go into the same report, marked `bridge:` |

To watch the lines themselves, set `LOG_LINES` to `1` in your firmware config:
the bridge's log on its USB serial port then shows every line as
`message: up: <line>` or `message: down: <line>`.

## A working example

[cloud-weather-logger](https://github.com/JehadAbdelBaqi/cloud-weather-logger)
is built this way:

| There | Is |
|-------|----|
| `scripts/bridge-config.example.sh` | Its script config |
| `firmware/include/config/bridge_config.h` | Its firmware config, filled from the headers its own firmware is built with |
| `scripts/build-config.sh` | Its build config, with the list of its own steps above the bridge's |
| `scripts/steps/build-and-upload-bridge.sh` | The step that runs the bridge's `e2e.sh` with those three files |
| `scripts/steps/test-nucleo.sh` and `test-connections.sh` | Its tests of both boards, through the broker |

## See also

[set-up-and-test.md](set-up-and-test.md) · [configuration.md](../system/configuration.md) · [architecture.md](../system/architecture.md) · [scripts/README.md](../../scripts/README.md) · [risks.md](../project-design/risks.md)
