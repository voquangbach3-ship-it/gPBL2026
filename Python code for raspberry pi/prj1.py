import RPi.GPIO as GPIO
from telegram.ext import ApplicationBuilder, MessageHandler, filters
from picamera2 import Picamera2
import time
import requests
from time import sleep
import threading
import serial
import cv2
import numpy as np

TOKEN = "8120771917:AAGBwURu68ZVwN8oczrReAizuPDE-VdQS14"
CHAT_ID = "8411566618"

LED = 4
LED2 = 22
VIB = 27
SERVO = 13

GPIO.setmode(GPIO.BCM)
GPIO.setup(LED, GPIO.OUT)
GPIO.setup(LED2, GPIO.OUT)
GPIO.setup(VIB, GPIO.IN)
GPIO.setup(SERVO, GPIO.OUT)

GPIO.output(LED, 0)
GPIO.output(LED2, 0)

pwm = GPIO.PWM(SERVO, 50)
pwm.start(0)

# Camera setup
picam2 = Picamera2()
picam2.configure(picam2.create_still_configuration())
picam2.start()
time.sleep(2)


# Serial
ser = None

def connect_serial():
    global ser
    try:
        ser = serial.Serial(
            '/dev/serial/by-id/usb-Arduino_UNO_WiFi_R4_CMSIS-DAP_D885ACA775B4-if01',
            9600,
            timeout=1
        )
        sleep(3)
        ser.reset_input_buffer()
        print("Serial connected successfully")
    except:
        print("No Arduino connected")
        ser = None

connect_serial()


def sendTelegram(text):
    try:
        url = f"https://api.telegram.org/bot{TOKEN}/sendMessage"
        data = {"chat_id": CHAT_ID, "text": text}
        requests.post(url, data=data, timeout=5)
    except Exception as e:
        print("Telegram error:", e)


def setAngle(angle):
    duty = 2.5 + (angle / 180) * 10
    pwm.ChangeDutyCycle(duty)
    sleep(0.5)
    pwm.ChangeDutyCycle(0)


def check_serial():
    global ser
    while True:

        if ser is None:
            connect_serial()
            sleep(2)
            continue

        try:
            if ser.in_waiting > 0:

                data = ser.readline().decode("utf-8").strip()
                print("Arduino:", data)

                if data == "Open the door":
                    GPIO.output(LED, 1)

                    sendTelegram("Password is correct!!! Do you want to open the door??")

                    sleep(3)
                    GPIO.output(LED, 0)
                elif data == "Wrong password":
                    sendTelegram("Password is uncorrect!!! Please do again!!!")

                elif data == "Block":
                    sendTelegram("BLOCKED SYSTEM FOR 10 MINUTES")
                    try:
                        image_path = "/home/team10/photo.jpg"
                        picam2.capture_file(image_path)
                
                        url = f"https://api.telegram.org/bot{TOKEN}/sendPhoto"
                
                        with open(image_path, "rb") as photo:
                            requests.post(
                                url,
                                files={"photo": photo},
                                data={"chat_id": CHAT_ID},
                                timeout=10
                            )
                
                    except Exception as e:
                        sendTelegram(f"Camera error: {e}")
                    
                elif data == "Taking a photo":
                    sendTelegram("Someone is standing at your door.")
                
                    try:
                        image_path = "/home/team10/photo.jpg"
                        picam2.capture_file(image_path)
                
                        url = f"https://api.telegram.org/bot{TOKEN}/sendPhoto"
                
                        with open(image_path, "rb") as photo:
                            requests.post(
                                url,
                                files={"photo": photo},
                                data={"chat_id": CHAT_ID},
                                timeout=10
                            )
                
                    except Exception as e:
                        sendTelegram(f"Camera error: {e}")

        except Exception as e:

            print("Serial error:", e)

            try:
                if ser:
                    ser.close()
            except:
                pass

            ser = None

        sleep(0.1)


def checkVibration():

    while True:

        if GPIO.input(VIB) == 1:

            sendTelegram("Hello there")

            sleep(5)

        sleep(0.1)



async def handle_message(update, context):

    text = update.message.text.lower()

    if text == "on":

        GPIO.output(LED2, 1)

        await update.message.reply_text("LED ON")

    elif text == "off":

        GPIO.output(LED2, 0)

        await update.message.reply_text("LED OFF")

    elif text == "open":

        setAngle(90)

        await update.message.reply_text("Door opened")

    elif text == "close":

        setAngle(0)

        await update.message.reply_text("Door closed")

    elif text == "take a photo":

        await update.message.reply_text("Taking a photo...")

        try:

            image_path = "/home/team10/photo.jpg"

            picam2.capture_file(image_path)

            with open(image_path, "rb") as photo:

                await update.message.reply_photo(photo=photo)

        except Exception as e:

            await update.message.reply_text(f"Camera error: {e}")


def main():

    app = ApplicationBuilder().token(TOKEN).build()

    app.add_handler(MessageHandler(filters.TEXT, handle_message))

    threading.Thread(target=check_serial, daemon=True).start()
    threading.Thread(target=checkVibration, daemon=True).start()

    print("Bot is running")

    app.run_polling()


if __name__ == '__main__':

    try:

        main()

    except KeyboardInterrupt:

        print("Stopping program")

    finally:

        try:

            pwm.stop()

            GPIO.cleanup()

            picam2.stop()

            if ser:
                ser.close()

        except:
            pass

        print("Done")
