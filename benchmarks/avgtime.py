import re
import csv
from sys import stdin

toTS = re.compile('[dhms]+')

def toS(t):
  assert 0 < len(t) <= 4
  if len(t) == 1:
    return float(t[0])
  elif len(t) == 2:
    return int(t[0]) * 60 + float(t[1])
  elif len(t) == 3:
    return int(t[0]) * 3600 + int(t[1]) * 60 + float(t[2])
  elif len(t) == 4:
    return  int(t[0]) * 86400 + int(t[1]) * 3600 \
           +int(t[2]) * 60 + float(t[2])

def add(stream):
  reader = csv.reader(stream, delimiter='\t', quotechar='"')
  n = 0
  ttot = 0
  tmax = 0
  for row in reader:
    ts = toTS.split(row[1])
    t = toS(ts)
    ttot += t
    tmax = max(tmax,t)
    n += 1
  return "t_avg = " + str(ttot/n) +"s, " + "t_max = " + str(tmax) + "s " \
                    + "(for " + str(n) + " samples)"
  
print(add(stdin))
