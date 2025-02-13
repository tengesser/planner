from itertools import product

def genEvents(n):
  result = ''
  events = list(product(*[[0,1,2] for i in range(n)]))
  for i in range(len(events)):
    e = events[i]
    foo = '(and\n'
    for s in range(n):
      if e[s]== 0: foo += '(knows ?a1 (not (secret-true s%d)))\n'%(s+1)
      if e[s]== 1: foo += '(knows ?a1 (secret-true s%d))\n'%(s+1)
      if e[s]== 2: foo += '(not (or (knows ?a1 (secret-true s%d)) (knows ?a1 (not (secret-true s%d)))))\n'%(s+1,s+1)
    foo += ')'  
    result += '''
(:event-designated kn%da1
   :precondition %s
   :effect (and))''' % (i,foo)
  return result

def genEventsS(n):
  result = ''
  events = list(product(*[[0,1,2] for i in range(2*n)]))
  for i in range(len(events)):
    e = events[i]
    foo = '(and\n'
    for s in range(n):
      if e[s]== 0: foo += '(knows ?a1 (not (secret-true s%d)))\n'%(s+1)
      if e[s]== 1: foo += '(knows ?a1 (secret-true s%d))\n'%(s+1)
      if e[s]== 2: foo += '(not (or (knows ?a1 (secret-true s%d)) (knows ?a1 (not (secret-true s%d)))))\n'%(s+1,s+1)
    for s in range(n):
      if e[n+s]== 0: foo += '(knows ?a2 (not (secret-true s%d)))\n'%(s+1)
      if e[n+s]== 1: foo += '(knows ?a2 (secret-true s%d))\n'%(s+1)
      if e[n+s]== 2: foo += '(not (or (knows ?a2 (secret-true s%d)) (knows ?a2 (not (secret-true s%d)))))\n'%(s+1,s+1)
    foo += ')'  
    result += '''
(:event-designated kn%d
   :precondition %s
   :effect (and))''' % (i,foo)
  return result

print(genEventsS(4))

#print(genEvents(4))
#print(genEvents(4).replace('a1','a2'))

#TODO
# generate whole domain/problem descriptions
# secretable propagation modes:
# single secret push or pull or both
# all secret push or pull or both

