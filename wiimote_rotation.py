import cwiid
from pynput.keyboard import Key, Controller
import time

keyboard = Controller()

def main():
    print("Press and hold the Wiimote's buttons to exit.")
    wii_remote = cwiid.Wiimote()

    wii_remote.rpt_mode = cwiid.RPT_ACC | cwiid.RPT_BTN

    while True:
        rotation = wii_remote.state['acc']
        print(rotation[1])
        t = 0.01
        if rotation[1] > 143.5:
            time.sleep(t)
            keyboard.release('d')
            keyboard.press('a')
        elif rotation[1] < 123.5:
            time.sleep(t)
            keyboard.release('a')
            keyboard.press('d')
        else:
            time.sleep(t)
            keyboard.release('a')
            keyboard.release('d')



if __name__ == "__main__":
    main()
