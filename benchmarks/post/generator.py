from random import sample, choice, randint
from itertools import product
from sys import argv, stderr, stdout

postTask = \
'''(define (problem post-problem)
  (:domain post)
  (:objects %s - agent
            %s - letter)
  (:init %s)%s
  (:observability none)
  (:goal %s))'''

def isConnected(l1,l2,agents,conns):
  # result: number of moves to reach goal
  steps = 0
  # generate successor dictionary
  c = {wp:set([]) for wp in agents}
  for (w1,w2) in conns:
    c[w1].add(w2)
  # setup connected and toSearchNext set
  connected = set([])
  toSearchNext = set([l1])
  while (toSearchNext != set([])):
    cNN = set([])
    while (toSearchNext != set([])):
      elem = toSearchNext.pop()
      if elem not in connected:
        connected.add(elem)
        cNN = cNN.union(c[elem]-connected)
    if l2 in connected:
      return steps
    else:
      toSearchNext = cNN
      steps += 1
  if len(connected) == len(agents):
    return -2
  else:
    return -1

def getWorld(n, nRoads):
  result = ()
  i = 0
  while(i < nRoads):
    n, x = divmod(n, 2)
    result = result + (x,)
    i += 1
  return result

def rv(x):
  (b,a) = x
  return (a,b)

def mix(l):
  return [(x,y) if randint(0,1) else (y,x) for (x,y) in l]

# nA   : number of agents
# nL   : number of letters
# rOWC : ratio of one way connections
# rTWC : ratio of two way connections
def generatePost(nA,nL,rOWC,rTWC):
  # agents and letters
  agents = ['A'+str(i+1) for i in range(0,nA)]
  letters = ['L'+str(i+1) for i in range(0,nL)]
  # one way and two way connections
  cands = set([(agents[i],agents[j]) for i in range(len(agents)) \
                                     for j in range(i+1,len(agents))])
  nPairs = len(cands)
  owc = set(sample(cands,int(rOWC * nPairs + 0.5)))
  cands = cands - owc
  twc = sample(cands,int(rTWC * nPairs + 0.5))
  connections = mix(owc) + list(twc) + list(map(rv,twc))
  connections.sort(key=lambda x: int(x[0][1:])*nA+int(x[1][1:]))
  # sender and receiver coordinates
  sender = [choice(agents) for l in letters]
  # recipients = [choice(agents) for l in letters]
  return (agents, letters, connections, sender) # , recipients

def generateInterestingPost(nA,nL,pTWC,pOWC):
  print('Generating',end='',file=stderr)
  while True:
    print('.',end='',file=stderr)
    post = generatePost(nA,nL,pTWC,pOWC)
    (agents, letters, connections, sender) = post
    # if not everybody is reachable, continue
    ok = True
    for s in sender:
      if isConnected(s,None,agents,connections) == -1:
        ok = False
        break
    if not ok:
      continue
    print(file=stderr)
    return post
  

def generatePostProb(post, pworlds, r):
  (agents, letters, connections, sender) = post
  nWorlds = len(agents)**len(letters)
  worlds = int(pworlds * nWorlds + 0.5)
  # objects
  aobs = ' '.join(agents)
  lobs = ' '.join(letters)
  # connections
  inits = ''
  inits += '(connected ' + connections[0][0] + ' ' + \
                           connections[0][1] + ')'
  for c in connections[1:]:
    inits += '\n         (connected ' + c[0] + ' ' + c[1] + ')'
  # senders = initial letter positions
  for i in range(len(letters)):
    inits += '\n         (at ' + letters[i] + ' ' + sender[i] + ')'
    for a in agents:
      if not sender[i] == a:
        inits += '\n         (not-at ' + letters[i] +' '+ a + ')'
  # worlds
  if r:
    ws = [sample(agents, len(letters)) for w in range(worlds)]
  else:
    candidates = list(product(*[agents for i in letters]))
    ws = sample(candidates, worlds)
  winits = ''
  for i in range(len(ws)):
    world = ws[i]
    winits += '\n  (:world-designated wtrue' if i == 0 else \
              '\n  (:world-nondesignated wfalse' + str(i)
    for i in range(len(letters)):
      winits += '\n    (destined ' + letters[i] + ' ' + world[i] + ')'
      for a in agents:
        if a != world[i]:
          winits += '\n    (not-destined ' + letters[i] + ' ' + a + ')'
    winits += ')'
  # and goal
  goal = '(and (received ' + letters[0] + ')'
  for l in letters[1:]:
    goal += '\n              (received ' + l + ')'
  goal += ')'
  return postTask % (aobs, lobs, inits, winits, goal)

def prod(iterable):
    return reduce(operator.mul, iterable, 1)

args = argv[1:]
if len(args) != 6:
  print ('usage:\n ... nAgents nLetters rOneWay rTwoWay rWorlds rand\nexample:\n  python3 generator.py 6 2 0.1 0.3 0.1 0')
else:
  print(generatePostProb(generateInterestingPost(int(args[0]), int(args[1]), float(args[2]), float(args[3])),float(args[4]),bool(int(args[5]))))


