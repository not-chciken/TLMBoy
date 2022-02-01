import gdb
import csv

class RegFile:
  def __init__(self):
    self.regs = {
      "af": 0x0000,
      "bc": 0x0000,
      "de": 0x0000,
      "hl": 0x0000,
      "sp": 0x0000,
      "pc": 0x0000,
    }

  def set_by_csv(self, csv_row):
    for tup in csv_row:
      kvs = tup.split(":")
      if kvs[0] in self.regs.keys():
        self.regs[kvs[0]] = int(kvs[1], 16)

  def set_by_gdb_str(self, gdb_str):
    tmp = [filter(None,t.split(" ")) for t in gdb_str.split("\n")]
    for kvs in tmp:
      kvs = list(kvs)
      try:
        if kvs[0] in self.regs.keys():
          self.regs[kvs[0]] = int(kvs[1], 16)
      except IndexError:
        continue

  def __getitem__(self, key):
    return self.regs[key]

  def __str__(self):
    return " ".join(k+":0x{:04x}".format(v) for k,v in self.regs.items())

  def __eq__(self, other):
    return all(v == other.regs[k] for k,v in self.regs.items())

reg_file_check = RegFile()
reg_file_golden = RegFile()

def check_dic(reg_file, reg_file_golden):
  if not (reg_file == reg_file_golden):
    print("Missmatch!")
    print(f"reg_file:        {reg_file}")
    print(f"reg_file_golden: {reg_file_golden}")
    exit(1)

print("starting gdb boot check test")
gdb.execute('set pagination off')
gdb.execute('set arch gbz80')
gdb.execute('target remote localhost:1337')
with open('golden_regs.csv') as csv_f:
  csv_read = csv.reader(csv_f, delimiter=',')
  ind = 0
  for row in csv_read:
    if ind > 500:
      break
    reg_file_golden.set_by_csv(row)
    gdb.execute(f"tbreak *{reg_file_golden['pc']}")
    gdb.execute("c")
    reg_str = gdb.execute('info all-registers', to_string=True)
    reg_file_check.set_by_gdb_str(reg_str)
    check_dic(reg_file_check, reg_file_golden)
    ind += 1

gdb.execute('quit')
print("boot test successful")