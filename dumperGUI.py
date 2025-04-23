import tkinter as tk
import serial
from tkinter import ttk

#Small GUI to select a com port and output file to write stream into
def main():
    root = tk.Tk()
    root.geometry("600x200")
    frame = ttk.Frame(root, padding=10)
    frame.pack(fill="both", expand=True)

    #COM dropdown
    ttk.Label(frame, text="Select COM Port:").grid(row=0, column=0, sticky="w")
    com_port = ttk.Combobox(frame, values=["COM1", "COM2", "COM3"], width=15)
    com_port.grid(row=0, column=1, pady=5, sticky="w")

    #File entry
    ttk.Label(frame, text="Output File Name:").grid(row=1, column=0, sticky="w")
    file_name = ttk.Entry(frame, width=40)
    file_name.grid(row=1, column=1, pady=5, sticky="w")

    #Executes on button click
    def on_start():
        port = com_port.get()
        baud_rate = 1_000_000
        n = 3
        name = file_name.get()
        with serial.Serial(port, baud_rate, timeout=n) as ser, open(name, 'wb') as outfile:
            try:
                while True:
                    chunk = ser.read(1024)
                    if not chunk:
                        #Nothing was recieved in (n) seconds
                        continue
                    outfile.write(chunk)
            except KeyboardInterrupt:
                print("Capture stopped")

    #Start button
    start_button = ttk.Button(frame, text="Start Dump", command=on_start)
    start_button.grid(row=2, column=0, columnspan=2, pady=20)

    root.mainloop()

if __name__ == "__main__":
    main()
