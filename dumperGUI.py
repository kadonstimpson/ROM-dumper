import tkinter as tk
from tkinter import ttk

def main():
    root = tk.Tk()
    root.geometry("600x200")
    frame = ttk.Frame(root, padding=10)
    frame.pack(fill="both", expand=True)

    #COM dropdown
    ttk.Label(frame, text="Select COM Port:").grid(row=0, column=0, sticky="w")
    com_port = ttk.Combobox(frame, values=["COM1", "COM2", "COM3"], width=15, state="readonly")
    com_port.grid(row=0, column=1, pady=5, sticky="w")

    #File entry
    ttk.Label(frame, text="Output File Name:").grid(row=1, column=0, sticky="w")
    file_name = ttk.Entry(frame, width=40)
    file_name.grid(row=1, column=1, pady=5, sticky="w")

    #Executes on button click
    def on_start():
        port = com_port.get()
        name = file_name.get()
        #TODO: set data equal to the dump over virtual com port
        data = "hello world!\n" #temp
        with open(name, "w", encoding="utf-8") as f: #TODO: This may need to be adjusted to account for writing a stream rather than a string
            f.write(data)

    #Start button
    start_button = ttk.Button(frame, text="Start Dump", command=on_start)
    start_button.grid(row=2, column=0, columnspan=2, pady=20)

    root.mainloop()

if __name__ == "__main__":
    main()
