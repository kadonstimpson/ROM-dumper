import tkinter as tk
import serial
from serial.tools import list_ports
from serial.tools.list_ports_common import ListPortInfo
from tkinter import ttk

#Small GUI to select a com port and output file to write stream into

# ---------- Helpers ----------

def available_ports() -> list[str]:
    return [p.device for p in list_ports.comports()]

def label(info: ListPortInfo) -> str:
    return f"{info.device} - {info.description or 'unknown'}" 

# ---------- GUI ----------

def main():
    root = tk.Tk()
    root.geometry("600x200")
    frame = ttk.Frame(root, padding=10)
    frame.pack(fill="both", expand=True)

    #COM dropdown
    ttk.Label(frame, text="Select COM Port:").grid(row=0, column=0, sticky="w")
    port_infos = list_ports.comports()
    port_labels = [label(p) for p in port_infos]
    port_map = {lbl: p.device for lbl, p in zip(port_labels, port_infos)}
    port_map = dict(zip(port_labels, (p.device for p in port_infos)))
    com_port = ttk.Combobox(frame, values=port_labels, width=40)
    com_port.grid(row=0, column=1, pady=5, sticky="w")
    if port_labels:             # Preselect 1st port
        com_port.current(0)

    #File entry
    ttk.Label(frame, text="Output File Name:").grid(row=1, column=0, sticky="w")
    file_name = ttk.Entry(frame, width=40)
    file_name.grid(row=1, column=1, pady=5, sticky="w")

    #Executes on button click
    def on_start():
        sel_label = com_port.get()  # Selection from list
        if not sel_label:
            tk.messagebox.showerror("No port", "Please select a serial port")
            return
        port = port_map[sel_label]  # Actaul raw port name
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
