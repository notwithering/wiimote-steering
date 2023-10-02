#include <stdio.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <unistd.h>
#include "lib/wiiuse.h"

#ifndef WIIUSE_WIN32
#include <unistd.h>
#endif

struct VirtualGameController {
    int uinput_fd;
};
int initVirtualController(struct VirtualGameController *controller, const char *devicePath) {
    controller->uinput_fd = open(devicePath, O_WRONLY | O_NONBLOCK);
    if (controller->uinput_fd < 0) {
        perror("Unable to open uinput device");
        return 1;
    }

    // Enable various input event types.
    ioctl(controller->uinput_fd, UI_SET_EVBIT, EV_KEY); // Enable key events.
    ioctl(controller->uinput_fd, UI_SET_EVBIT, EV_ABS); // Enable absolute events.
    ioctl(controller->uinput_fd, UI_SET_EVBIT, EV_SYN); // Enable SYN events (sync).

    // Common controller buttons.
    ioctl(controller->uinput_fd, UI_SET_KEYBIT, BTN_A);
    ioctl(controller->uinput_fd, UI_SET_KEYBIT, BTN_B);
    ioctl(controller->uinput_fd, UI_SET_KEYBIT, BTN_X);
    ioctl(controller->uinput_fd, UI_SET_KEYBIT, BTN_Y);
    ioctl(controller->uinput_fd, UI_SET_KEYBIT, BTN_START);
    ioctl(controller->uinput_fd, UI_SET_KEYBIT, BTN_SELECT);
    ioctl(controller->uinput_fd, UI_SET_KEYBIT, BTN_TRIGGER_HAPPY1);
    ioctl(controller->uinput_fd, UI_SET_KEYBIT, BTN_TRIGGER_HAPPY2);

    // Enable axes.
    ioctl(controller->uinput_fd, UI_SET_ABSBIT, ABS_X);   // X-axis.
    ioctl(controller->uinput_fd, UI_SET_ABSBIT, ABS_Y);   // Y-axis.
    ioctl(controller->uinput_fd, UI_SET_ABSBIT, ABS_Z);   // ABS_Z (left trigger).
    ioctl(controller->uinput_fd, UI_SET_ABSBIT, ABS_RZ);  // ABS_RZ (right trigger).
    ioctl(controller->uinput_fd, UI_SET_ABSBIT, ABS_RX);  // Right stick X-axis.
    ioctl(controller->uinput_fd, UI_SET_ABSBIT, ABS_RY);  // Right stick Y-axis.
    ioctl(controller->uinput_fd, UI_SET_ABSBIT, ABS_HAT0X); // D-pad X-axis.
    ioctl(controller->uinput_fd, UI_SET_ABSBIT, ABS_HAT0Y); // D-pad Y-axis.

    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Wiimote Steering");
    uidev.id.bustype = BUS_VIRTUAL;
    uidev.id.vendor = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;

    if (write(controller->uinput_fd, &uidev, sizeof(uidev)) < 0) {
        perror("Failed to write uinput_user_dev");
        close(controller->uinput_fd);
        return 1;
    }

    if (ioctl(controller->uinput_fd, UI_DEV_CREATE) < 0) {
        perror("Failed to create uinput device");
        close(controller->uinput_fd);
        return 1;
    }

    return 0;
}

void cleanupVirtualController(struct VirtualGameController *controller) {
    ioctl(controller->uinput_fd, UI_DEV_DESTROY);
    close(controller->uinput_fd);
}

void handle_ctrl_status(struct wiimote_t* wm) {
	printf("\n\n--- CONTROLLER STATUS [wiimote id %i] ---\n", wm->unid);

	printf("attachment:      %i\n", wm->exp.type);
	printf("speaker:         %i\n", WIIUSE_USING_SPEAKER(wm));
	printf("ir:              %i\n", WIIUSE_USING_IR(wm));
	printf("leds:            %i %i %i %i\n", WIIUSE_IS_LED_SET(wm, 1), WIIUSE_IS_LED_SET(wm, 2), WIIUSE_IS_LED_SET(wm, 3), WIIUSE_IS_LED_SET(wm, 4));
	printf("battery:         %f %%\n", wm->battery_level);
}

short any_wiimote_connected(wiimote** wm, int wiimotes) {
	int i;
	if (!wm) {
		return 0;
	}

	for (i = 0; i < wiimotes; i++) {
		if (wm[i] && WIIMOTE_IS_CONNECTED(wm[i])) {
			return 1;
		}
	}

	return 0;
}

// int argc, char** argv
int main() {
	const char *devicePath = "/dev/uinput";
    struct VirtualGameController controller;

	if (initVirtualController(&controller, devicePath) != 0) {
        fprintf(stderr, "Failed to initialize virtual game controller.\n");
        return 1;
    }


	wiimote** wiimotes;
	int found, connected;

	wiimotes =  wiiuse_init(2);

	found = wiiuse_find(wiimotes, 2, 5);
	if (!found) {
		printf("No wiimotes found.\n");
		return 0;
	}


	connected = wiiuse_connect(wiimotes, 2);
	if (connected) {
		printf("Connected to %i wiimotes (of %i found).\n", connected, found);
	} else {
		printf("Failed to connect to any wiimote.\n");
		return 0;
	}

	if (wiimotes[0]->exp.type != 0 || wiimotes[1]->exp.type != 4) {
		if (wiimotes[1]->exp.type != 0 || wiimotes[0]->exp.type != 4) {
			printf("One or more devices incorrect.\n");
			printf("%i, %i (%i, %i)", wiimotes[0]->exp.type, wiimotes[1]->exp.type, 0, 4);
			return 0;
		}
	}
	
	wiiuse_set_leds(wiimotes[0], WIIMOTE_LED_NONE);
	wiiuse_set_leds(wiimotes[1], WIIMOTE_LED_NONE);
	wiiuse_motion_sensing(wiimotes[0], 1);
	wiiuse_motion_sensing(wiimotes[1], 1);
	wiiuse_rumble(wiimotes[0], 1);
	wiiuse_rumble(wiimotes[1], 1);

	#ifndef WIIUSE_WIN32
		usleep(200000);
	#else
		Sleep(200);
	#endif

	wiiuse_rumble(wiimotes[0], 0);
	wiiuse_rumble(wiimotes[1], 0);

	while (any_wiimote_connected(wiimotes, 2)) {
		if (wiiuse_poll(wiimotes, 2)) {
			struct input_event ev;
			memset(&ev, 0, sizeof(ev));

			int i = 0;
			for (; i < 2; ++i) {
				struct wiimote_t* wm = wiimotes[i];

				if (wm->exp.type == EXP_WII_BOARD) {
					struct wii_board_t* wb = (wii_board_t*)&wm->exp.wb;
					if (wb->tl == 0 && wb->tr > 0) {
						ev.type = EV_KEY;
						ev.code = BTN_TL; // Left bumper
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));

						ev.type = EV_ABS;
						ev.code = ABS_Z; // Left trigger
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
						//
						ev.type = EV_ABS;
						ev.code = ABS_RZ; // Right trigger
						ev.value = wb->tr / 3 * 32767;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else if (wb->tl > 0 && wb->tr > 0) {
						ev.type = EV_KEY;
						ev.code = BTN_TL; // Left bumper
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));

						ev.type = EV_ABS;
						ev.code = ABS_RZ; // Right trigger
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
						//
						ev.type = EV_ABS;
						ev.code = ABS_Z; // Left trigger
						ev.value = wb->tl / 3 * 32767;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else if (wb->tl > 0 && wb->tr == 0) {
						ev.type = EV_ABS;
						ev.code = ABS_Z; // Left trigger
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));

						ev.type = EV_ABS;
						ev.code = ABS_RZ; // Right trigger
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
						//
						ev.type = EV_KEY;
						ev.code = BTN_TL; // Left bumper
						ev.value = 1;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else {
						ev.type = EV_KEY;
						ev.code = BTN_TL; // Left bumper
						ev.value = 1;
						write(controller.uinput_fd, &ev, sizeof(ev));

						ev.type = EV_ABS;
						ev.code = ABS_Z; // Left trigger
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));

						ev.type = EV_ABS;
						ev.code = ABS_RZ; // Right trigger
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
					}
				}

				if (wm->exp.type == 0) {
					ev.type = EV_ABS;
					ev.code = ABS_X; // Left joystick X-axis
					ev.value = wm->orient.pitch / 85 * -32767;
					write(controller.uinput_fd, &ev, sizeof(ev));

					ev.code = ABS_Y; // Left joystick Y-Axis
					ev.value = 0;
					write(controller.uinput_fd, &ev, sizeof(ev));

					if (IS_PRESSED(wm, WIIMOTE_BUTTON_HOME))	{
						handle_ctrl_status(wm);
					}
					if (IS_PRESSED(wm, WIIMOTE_BUTTON_UP)) {
						ev.type = EV_ABS;
						ev.code = ABS_HAT0X; // D-pad X-axis
						ev.value = -1;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else {
						ev.type = EV_ABS;
						ev.code = ABS_HAT0X; // D-pad X-axis
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
					}
					if (IS_PRESSED(wm, WIIMOTE_BUTTON_DOWN)) {
						ev.type = EV_ABS;
						ev.code = ABS_HAT0X; // D-pad X-axis
						ev.value = 1;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else {
						ev.type = EV_ABS;
						ev.code = ABS_HAT0X; // D-pad X-axis
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
					}
					if (IS_PRESSED(wm, WIIMOTE_BUTTON_LEFT)) {
						ev.type = EV_ABS;
						ev.code = ABS_HAT0Y; // D-pad Y-axis
						ev.value = 1;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else {
						ev.type = EV_ABS;
						ev.code = ABS_HAT0Y; // D-pad Y-axis
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
					}
					if (IS_PRESSED(wm, WIIMOTE_BUTTON_RIGHT)) {
						ev.type = EV_ABS;
						ev.code = ABS_HAT0Y; // D-pad Y-axis
						ev.value = -1;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else {
						ev.type = EV_ABS;
						ev.code = ABS_HAT0Y; // D-pad Y-axis
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
					}
					if (IS_PRESSED(wm, WIIMOTE_BUTTON_TWO)) {
						ev.type = EV_KEY;
						ev.code = BTN_X;
						ev.value = 1;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else {
						ev.type = EV_KEY;
						ev.code = BTN_X;
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
					}
					if (IS_PRESSED(wm, WIIMOTE_BUTTON_ONE)) {
						ev.type = EV_KEY;
						ev.code = BTN_A;
						ev.value = 1;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else {
						ev.type = EV_KEY;
						ev.code = BTN_A;
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
					}
					if (IS_PRESSED(wm, WIIMOTE_BUTTON_A)) {
						ev.type = EV_KEY;
						ev.code = BTN_START;
						ev.value = 1;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else {
						ev.type = EV_KEY;
						ev.code = BTN_START;
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
					}
					if (IS_PRESSED(wm, WIIMOTE_BUTTON_B)) {
						ev.type = EV_KEY;
						ev.code = BTN_TR; // Right bumper
						ev.value = 1;
						write(controller.uinput_fd, &ev, sizeof(ev));
					} else {
						ev.type = EV_KEY;
						ev.code = BTN_TR; // Right bumper
						ev.value = 0;
						write(controller.uinput_fd, &ev, sizeof(ev));
					}
				}

				ev.type = EV_SYN;
				ev.code = SYN_REPORT;
				ev.value = 0;
				write(controller.uinput_fd, &ev, sizeof(ev));

				switch (wm->event) {
					case WIIUSE_EVENT:
						break;
					case WIIUSE_STATUS:
						handle_ctrl_status(wiimotes[i]);
						break;

					case WIIUSE_DISCONNECT:
					case WIIUSE_UNEXPECTED_DISCONNECT:
						cleanupVirtualController(&controller);
						return 0;

					default:
						break;
				}
			}
		}
	}

	wiiuse_cleanup(wiimotes, 2);

	return 0;
}
