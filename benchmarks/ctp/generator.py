from itertools import product
from random import sample, random, randint, choice
from sys import argv, stderr, stdout
from os import devnull

dbgprnt = open(devnull, 'w')

ctpTask = \
'''(define (problem ctp-problem)
  (:domain ctp)
  (:objects %s - agent
            %s - waypoint)
  (:init %s)%s
  (:observability none)
  (:goal %s))'''

def rv(x):
  (b,a) = x
  return (a,b)

def getWorld(n, nRoads):
  result = ()
  i = 0
  while(i < nRoads):
    n, x = divmod(n, 2)
    result = result + (x,)
    i += 1
  return result

def mix(l):
  return [(x,y) if randint(0,1) else (y,x) for (x,y) in l]

def gf(worlds):
  worlds = list(worlds)
  s = list(worlds[0])
  for w in worlds:
    for i in range(len(w)):
      s[i] &= w[i]
  return tuple(s)

def isConnected(wP1,wp2,wps,roads):
  # result: number of moves to reach goal
  steps = 0
  # generate successor dictionary
  c = {wp:set([]) for wp in wps}
  for (w1,w2) in roads:
    c[w1].add(w2)
  # setup connected and toSearchNext set
  connected = set([])
  toSearchNext = set([wP1])
  while (toSearchNext != set([])):
    cNN = set([])
    while (toSearchNext != set([])):
      elem = toSearchNext.pop()
      if elem not in connected:
        connected.add(elem)
        cNN = cNN.union(c[elem]-connected)
    if wp2 in connected:
      return steps
    else:
      toSearchNext = cNN
      steps += 1
  return -1

# nA:  #Agents
# nWP: #Waypoints
# rOW: ratio of overall possible connections that will form one way roads
# rTW: ratio of overall possible connections that will form two way roads
# nUK: number of roads that may be blocked
# nWs: number of different world (blocked road) configurations
# minPL: minimal allowed path length to the goal node
# Criterion 1: all possible start-goal paths have to have at leats minPL length
# Criterion 2: each unknown road has to possibly avert an existing path
# Criterion 3: each world configuration has to have an existing path
def genCTP(nA,nWP,rOW,rTW,nUK,nW,minPL):
  print('Generating',file=stderr,end='')
  while True:
    print(':',file=dbgprnt,end='')
    # agents
    agents = ['A'+str(i+1) for i in range(0,nA)]
    # waypoints
    waypoints = ['W'+str(i+1) for i in range(0,nWP)]
    # roads
    roadCands = set([(waypoints[i], waypoints[j]) for i in range(0,nWP) \
                                                  for j in range(0,i)])
    nConnections = len(roadCands)
    roadsOW = set(sample(roadCands, int(rOW * nConnections + 0.5)))
    twCands = roadCands - roadsOW
    roadsTW = set(sample(twCands, int(rTW * nConnections + 0.5)))
    roads = mix(roadsOW) + list(roadsTW) + list(map(rv,roadsTW))
    roads.sort(key=(lambda x: int(x[0][1:]*(nA+1)+x[1][1:])))
    # check, if there exists a way and its not to short
    pl = isConnected(waypoints[0],waypoints[len(waypoints)-1], \
                     waypoints, roads)
    if pl < minPL:
      continue
    # now determine unknown roads
    pl = 0
    nTries = 0
    print('?',file=dbgprnt,end='')
    while nTries < int(nWP*nWP) and pl >= 0:
      print('.',file=dbgprnt,end='')
      unknown = sample(roads, nUK)
      pl = isConnected(waypoints[0],waypoints[len(waypoints)-1], \
                       waypoints, set(roads)-set(unknown))
      nTries += 1
    if pl == 0: continue
    # now try to find interesting world examples
    worlds = set([])
    nTries = 0
    print('!',file=dbgprnt,end='')
    while nTries < int(nWP*nWP) and \
          len(worlds) < nW:
      print('.',file=dbgprnt,end='')
      w = randint(0,2**nUK-1)
      world = getWorld(w,nUK)
      # guarantee that there is a solution for the world
      config = (set(roads) - set(unknown)) | \
               set([unknown[i] for i in range(len(unknown)) \
                                     if world[i] == 1])
      if isConnected(waypoints[0],waypoints[len(waypoints)-1], \
                     waypoints, config) == -1:
        continue
      worlds |= set([world])
      nTries += 1
    if len(worlds) < nW:
      continue
    gfworld = gf(worlds)
    configgf = (set(roads) - set(unknown)) | \
                set([unknown[i] for i in range(len(unknown)) \
                                      if gfworld[i] == 1])
    if isConnected(waypoints[0],waypoints[len(waypoints)-1], \
                   waypoints, configgf) != -1:
      continue
    return (agents, waypoints, roads, unknown, worlds)

def genCTPMAEPL(can,com):
  (agents, waypoints, roads, unknown, worlds) = can
  # objects
  wpobs = ' '.join(waypoints)
  agobs = ' '.join(agents)
  # global init configuration
  init = '' if com else '; '
  init += '(communication-enabled)'
  # agent positions
  for a in agents:
    init += '\n         (at ' + a + ' ' + waypoints[0] + ')'
    for wp in waypoints[1:]:
      init += '\n         (not-at ' + a + ' ' + wp + ')'
  # roads
  for i in range(len(roads)):
    init += '\n         '
    init += '(road ' + roads[i][0] + ' ' + roads[i][1] + ')'
    if roads[i] not in unknown:
      init += ' (not-blocked ' + roads[i][0] + ' ' + roads[i][1] + ')'
  # the worlds: different unknown road combinations
  worlds = list(worlds)
  desig = randint(0,len(worlds)-1)
  wconf = ''
  for j in range(len(worlds)):
    wconf += '\n  (:world-' + ('designated' if j == desig else 'nondesignated') + ' w' + str(j)
    for i in range(len(worlds[j])):
      if worlds[j][i]:
        wconf += '\n    (not-blocked ' + unknown[i][0] +' '+ unknown[i][1] + ')'
      else:
        wconf += '\n    (blocked ' + unknown[i][0] +' '+ unknown[i][1] + ')'
    wconf += ')'
  goal = '(at A1 ' + waypoints[len(waypoints)-1] + ')'
  print(file=stderr)
  return ctpTask % (agobs, wpobs, init, wconf, goal)

def prod(iterable):
    return reduce(operator.mul, iterable, 1)

helpstr = \
'''usage:
    ... nAgents nWaypoints rOneWay rTwoWays nUnknown nWs minPL com
example:
    python3 generator.py 2 5 0.6 0 3 2 2 1'''


args = argv[1:]
if len(args) != 8:
  print (helpstr)
else:
  print(genCTPMAEPL(genCTP(int(args[0]), int(args[1]), float(args[2]), float(args[3]), int(args[4]), int(args[5]), int(args[6])),float(args[7]) >= 0.5))

