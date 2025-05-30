#!/usr/bin/env python3
"""
Simple script to send temperature and humidity readings to Flask API
"""

import random
import time

import requests


def send_reading():
    # Generate random realistic values
    temperatura = round(random.uniform(15.0, 35.0), 1)
    humedad = round(random.uniform(40.0, 80.0), 1)

    data = {"temperatura": temperatura, "humedad": humedad}

    try:
        response = requests.post(
            "http://localhost:5000/lectura", json=data, headers={"Content-Type": "application/json"}
        )

        if response.status_code == 201:
            print(f"✅ Sent: {temperatura}°C, {humedad}%")
        else:
            print(f"❌ Error: {response.status_code}, {response.text}")

    except Exception as e:
        print(f"❌ Failed: {e}")


if __name__ == "__main__":
    # Send 10 readings
    for i in range(10):
        print(f"Reading {i+1}/10")
        send_reading()
        time.sleep(1)

    print("✅ Done!")
