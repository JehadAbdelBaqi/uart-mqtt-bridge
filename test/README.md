# Unit tests

Tests for the bridge's application code. They are built for the PC and run on
it, with no board plugged in.

[← README](../README.md)

## Running them

From the top of the repository:

```
pio test -e native
```

One test folder alone:

```
pio test -e native -f app/test_upstream_messaging
```

## What is tested

The application's files under `src/app/`: what the bridge does with a message.
There is one test folder for each.

```
test/
  app/   test_line_framing/            bytes from the MCU to a whole message
         test_helpers/                 whether a message can be passed on, making one ready
                                       to send to the MCU, the message log
         test_status/                  which status is the case, and the text of each
         test_downstream_messaging/    the bridge's answer to a message, the handshake's answer
         test_message_routing/         a handshake answered, every other message passed on
         test_upstream_messaging/      the topic for a message, holding it until the broker
                                       confirms it, the time limit, messages from the broker
         test_downstream_connection/   the MCU shown as connected, quiet or gone
  fakes/                               stand-ins for what only exists on the board
```

## What is not tested here

| Code | Why |
|------|-----|
| The modules (`components/modules/`) | They operate the UART, Wi-Fi, the MQTT client and the LED, so they only run on the board |
| `main.c` | It only starts the modules and the application |
| `dummy_source.c` | It is a task that writes to the UART on a timer |
| The task that reads the UART | It loops for ever; the functions it calls are tested |
| Two tasks running at the same time | A test runs one thing at a time |

The end-to-end test through the broker covers the board as a whole.

## How it works

- **Unity** is the test framework. PlatformIO fetches it and runs the tests.
- **Each test folder is one small program.** PlatformIO builds it with the
  PC's compiler, runs it and reads the result of each test from its output.
- **The `native` environment in `platformio.ini` lists the files to compile.**
  Only those are built for the PC.
- **`test/fakes/` stands in for the board.** The application calls the UART
  link, the MQTT link, the Wi-Fi link, the LED, the timers and the log. On the
  PC each of those is a fake that records what it was asked to do, and a test
  reads that back. A fake can also be told what to answer: whether Wi-Fi is
  up, which ID a published message gets.
- **Nothing happens by itself.** No timer runs out and no time passes unless a
  test says so, with `fake_timer_fire()` and `fake_set_time_ms()`. The broker
  confirms a message when a test calls `fake_mqtt_confirm()`.
- **The tests have a config of their own,**
  `test/fakes/generated/config_in_use.h`, in place of the one the build script
  writes. A change to a real config does not change what the tests expect. Its
  line limit is short, so a test reaches it with a few characters. The
  `native` environment names `test/fakes/` with `-iquote`, so that it is
  searched before the project's own `include/`, where the real one is.
- **A test file has the same shape every time:** `setUp()` runs before each
  test, one function per case, and `main()` lists the cases to run.

## Adding a test

1. In `platformio.ini`, under `[env:native]`: add the file under test to
   `build_src_filter`, and the folder its header is in to `build_flags` if it
   is not there yet.
2. If the file calls something new that only exists on the board, add a fake
   for it to `test/fakes/fakes.c`, and what a test needs of it to `fakes.h`.
3. Add `test/app/test_<file>/test_<file>.c`.
4. Run `pio test -e native`.
