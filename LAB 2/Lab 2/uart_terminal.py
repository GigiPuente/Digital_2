import sys
import threading
import time

try:
    import serial
except ImportError:
    print("Falta pyserial. Instala con: pip install pyserial")
    raise


def leer_serial(ser):
    while True:
        try:
            if ser.in_waiting:
                linea = ser.readline().decode("utf-8", errors="replace").strip()
                if linea:
                    print(f"RX: {linea}")
            time.sleep(0.05)
        except Exception as e:
            print(f"Error al leer: {e}")
            break


def main():
    if len(sys.argv) < 2:
        print("Uso: python uart_terminal.py COM3 [9600]")
        return

    puerto = sys.argv[1]
    baudios = 9600 if len(sys.argv) < 3 else int(sys.argv[2])

    ser = serial.Serial(puerto, baudios, timeout=0.1)
    print(f"Conectado a {puerto} a {baudios} baudios")
    print("Escribe + o - y presiona Enter. Ctrl+C para salir.")

    hilo = threading.Thread(target=leer_serial, args=(ser,), daemon=True)
    hilo.start()

    try:
        while True:
            txt = input("TX: ")
            if txt:
                ser.write(txt.encode("utf-8"))
    except KeyboardInterrupt:
        print("\nCerrando puerto...")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
