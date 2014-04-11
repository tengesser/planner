from sys import argv
from itertools import product
from random import sample, randint

problem = '''(define (problem mastermind-task)
  (:domain mastermind)
  (:objects %s - color)
  (:init (not (win))) %s
  (:observability none)
  (:goal (win)))'''

def genProb(nC, pWorlds):
  colors = ["C"+str(i) for i in range(nC)]
  configs = list(product(colors, colors, colors, colors))
  worlds = sample(configs, int(pWorlds*len(configs)+0.5))
  worlds.sort()
  wdes = randint(0,len(worlds)-1)
  return (colors, worlds, wdes)

def genOP(task):
  (colors, worlds, wdes) = task
  opclist = ' '.join(colors)
  opwlist = ''
  for i in range(len(worlds)):
    w = worlds[i]
    opwlist += ('\n  (:world-designated' if i == wdes else \
                '\n  (:world-nondesignated') + ' world' + str(i+1)
    opwlist += '\n     (c1 ' + w[0] +')'
    opwlist += '\n     (c2 ' + w[1] +')'
    opwlist += '\n     (c3 ' + w[2] +')'
    opwlist += '\n     (c4 ' + w[3] +'))'
  return problem % (opclist, opwlist)

args = argv[1:]
if len(args) != 2:
  print ('parameters:\n  nColors pWorld\nexample:\n  python3 generator.py 3 0.2')
else:
  print(genOP(genProb(int(args[0]), float(args[1]))))



  
  
