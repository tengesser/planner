import csv
from sys import stdin

def add(stream):
  result = (0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)
  reader = csv.reader(stream, delimiter='\t', quotechar='"')
  n = 0
  for row in reader:
    result = map(sum, zip(result, map(int,row[1:])))
    n += 1
  return [int(round(float(r)/n)) for r in result] + [n]

def op(t):
  return '\t'.join(map(str,t))
  
print(op(add(stdin)))
