import math
import cmath
import array
import numpy
import matplotlib.pyplot as plt

use_numpy_for_processing = False#True

def dbl(x):
    return math.pow(10.0, x/20.0)

def zeros(dims):
    global use_numpy_for_processing
    if use_numpy_for_processing:
        return numpy.zeros(dims)
    else:
        if not dims:
            return 0.0
        else:
            return arrayof([0.0 for _ in range(dims[0])])

def arrayof(xs, t='d'):
    global use_numpy_for_processing
    if use_numpy_for_processing:
        return numpy.array(xs)
    else:
        return array.array(t, xs)

class Filter:
    def __init__(self):
        self.s1 = 0.0
        self.s2 = 0.0
        self.hp = 0.0
        self.bp = 0.0
        self.lp = 0.0
        self.br = 0.0
        self.ap = 0.0
        self.setparams(0.25, 0.5, 1.0)
    
    def setparams(self,k,q,sr):
        self.sr = sr
        self.k = math.tan(math.pi * k / sr)
        self.q = q
        self.d = 1.0 + k/q + k*k
        
    def process(self, x:float):
        self.hp = (x - (1.0 / self.q + self.k) * self.s1 - self.s2) / self.d
        u = self.hp * self.k
        self.bp = u + self.s1
        self.s1 = u + self.bp
        u = self.bp * self.k
        self.lp = u + self.s2
        self.s2 = u + self.lp
        self.br = self.hp + self.lp
        self.ap = self.br + (1.0 / self.q) * self.bp

class BiQuad:
    def __init__(self) -> None:
        self.delay = zeros((4,))
    
    def process(self, x, g, b1, b2, a1, a2):
        y = g*x + b1 * self.delay[0] + b2 * self.delay[1] - a1 * self.delay[2] - a2 * self.delay[3]
        self.delay[1] = self.delay[0]
        self.delay[0] = x
        self.delay[3] = self.delay[2]
        self.delay[2] = y
        return y

class BQFilter:
    def __init__(self, n) -> None:
        self.B = [BiQuad() for _ in range(n)]
        self.n = n
        self.lp = 0.0
        self.setparams(.25, .625)
    
    def setparams(self, k, q):
        self.k = k
        self.q = q
        w = 2*math.pi * k
        a = math.sin(w) / (2.*q)
        t = math.cos(w)
        self.a0 = (1.+a)
        self.a1 = -2. * t
        self.a2 = 1. - a
        self.b0 = (1. - t)/2.
        self.b1 = 1. - t
        self.b2 = (1. - t)/2.
        self.a1,self.a2,self.b1,self.b2 = self.a1/self.a0,self.a2/self.a0,self.b1/self.a0,self.b2/self.a0
        self.g = self.b0 / self.a0
        
    def process(self, x):
        for i in range(self.n):
            x = self.B[i].process(x, self.g, self.b1, self.b2, self.a1, self.a2)
        self.lp = x
        return x
    
    def reset(self):
        for i in range(self.n):
            self.B[i].delay = zeros((4,))

class Amp:
    def __init__(self, gain=1., drive1=1., drive2=1., factor=2, stages=1, compensation=0., resonance=1., blend=1., iterations=16, internal_factor=4, use_lut=False, max_factor=16) -> None:
        self.gain = gain
        self.drive1 = drive1
        self.drive2 = drive2
        self.factor = factor
        self.stages = stages
        self.compensation = compensation
        self.resonance = resonance
        self.eps = blend
        self.filters = [[BQFilter(8) for _ in range(4)] for _ in range(self.stages)]
        self.highpass = [Filter() for _ in range(self.stages)]
        self.gains = [BQFilter(1) for _ in range(self.stages)]
        self.iterations = iterations
        self.internal_factor = internal_factor
        self.use_lut = use_lut
        if self.use_lut:
            self.umax = 384.#4096.
            self.ustep = 16
            self.gmax = 2.
            self.gstep = 8
            self.tmax = 2.
            self.tstep = 2
            self.lut = zeros((self.tstep * self.gstep * self.ustep,))
            for t in range(0,self.tstep):
                for g in range(0,self.gstep):
                    for u in range(0, self.ustep):
                        x = self.fc(u / self.ustep * self.umax, g / self.gstep * self.gmax, t / self.tstep * self.tmax)
                        self.lut[t*self.gstep*self.ustep+g*self.ustep+u] = x
            self.fc = self.fc_lut
        self.max_factor = max_factor
    
    def fc_lut(self, u, g, tn=.1):
        tn = abs(tn) / self.tmax * self.tstep
        r = tn - math.floor(tn)
        i = int(tn)
        if i > self.tstep-2:
            i = self.tstep-2
        g = abs(g) / self.gmax * self.gstep
        s = g - math.floor(g)
        j = int(g)
        if j > self.gstep-2:
            j = self.gstep-2
        u = abs(u) / self.umax * self.ustep
        t = u - math.floor(u)
        k = int(u)
        if k > self.ustep-2:
            k = self.ustep-2
        return ((self.lut[(i)*(self.gstep*self.ustep)+(j)*self.ustep+(k)]*(1.-t) + t*self.lut[(i)*(self.gstep*self.ustep)+(j)*self.ustep+(k+1)])*(1.-s) + s*(self.lut[(i)*(self.gstep*self.ustep)+(j+1)*self.ustep+(k)]*(1.-t) + t*self.lut[(i)*(self.gstep*self.ustep)+(j+1)*self.ustep+(k+1)]))*(1.-r) + \
                ((self.lut[(i+1)*(self.gstep*self.ustep)+(j)*self.ustep+(k)]*(1.-t) + t*self.lut[(i+1)*(self.gstep*self.ustep)+(j)*self.ustep+(k+1)])*(1.-s) + s*(self.lut[(i+1)*(self.gstep*self.ustep)+(j+1)*self.ustep+(k)]*(1.-t) + t*self.lut[(i+1)*(self.gstep*self.ustep)+(j+1)*self.ustep+(k+1)]))*r

    def f(self, x, limit):
        return limit * x / (1. + abs(x))
        
    def fc(self, u, g, tension=.1):
        internal_factor = self.internal_factor
        bf = zeros((internal_factor,))
        k = 0
        fl = BQFilter(8)
        cutoff = 0.5 / internal_factor
        Q = 0.700
        fl.setparams(cutoff, Q)
        while k < self.iterations:
            s = 0.0
            fl.reset()
            fl.process(g * u)
            bf[0] = fl.lp
            for i in range(1,internal_factor):
                fl.process(0.0)
                bf[i] = fl.lp
            for i in range(0,internal_factor):
                bf[i] = self.f(bf[i] * internal_factor, 1.)
            fl.reset()
            for i in range(0,internal_factor):
                fl.process(bf[i])
                s += abs(abs(bf[i])-abs(fl.lp))
            if abs(s) > tension * 1.e-3:
                if g - g * .5 / (1 << k) >= 0.0:
                    g -= g * .5 / (1 << k)
            else:
                break
            k += 1
        return g
    
    def process(self, x:float) -> float:
        t = x
        sampling = self.factor
        sampling4x = 4
        buffer = zeros((self.max_factor,))

        for h in range(self.stages):
            for i in range(0,2):
                self.filters[h][i].setparams(0.5 / sampling, self.resonance)
            for i in range(2,4):
                self.filters[h][i].setparams(0.5 / sampling4x, self.resonance)
            self.highpass[h].setparams(.0001 * 48000.0 / sr,0.9,1.0)
            self.gains[h].setparams(k = 0.0001 * 48000.0 / sr, q = .625)
        
        def nonlin(u,h):
            s = float(u > 0) - float(u < 0)
            u = abs(u)
            v = u
            v = v * (1. - self.drive2) + self.drive2 * (v + v*(1. - v))
            u = u * (1. - self.drive1) + self.drive1 * (u + u**2*(1. - u))
            u = (1.-self.eps*h/self.stages) * u + (self.eps*h/self.stages) * v
            u = u * s
            return u
        
        t = t * dbl(self.gain)
        for h in range(self.stages):
            buffer[0] = self.filters[h][0].process(t)
            for j in range(1,sampling):
                buffer[j] = self.filters[h][0].process(0.0)
            
            for j in range(sampling):
                t = buffer[j] * sampling
                self.gains[h].process(1.0)
                g = self.fc(t, abs(self.gains[h].lp))
                self.gains[h].process(g)
                g = abs(self.gains[h].lp)
                t = self.f(g * t, 1.)
                buffer[j] = t

            for j in range(sampling):
                t = buffer[j]
                t = self.filters[h][1].process(t)
            
            # polynomial shaping
            buffer[0] = self.filters[h][2].process(t)
            for j in range(1,sampling4x):
                buffer[j] = self.filters[h][2].process(0.0, )
            for j in range(sampling4x):
                buffer[j] = nonlin(buffer[j] * sampling4x, h)
            for j in range(sampling4x):
                t = self.filters[h][3].process(buffer[j])
            # end polynomial shaping

            self.highpass[h].process(t)
            t = self.highpass[h].hp
            t = t * dbl(self.compensation)
        
        return t


n = 4096#2**16
sr = float(n)
f = 6.
print(f)
##sine = arrayof([0.5*math.sin(math.pi * f * 2.0 * (i/n)) + 0.25*math.sin(math.pi * 2.*f * 2.0 * (i/n)) + 0.1666*math.sin(math.pi * 3.*f * 2.0 * (i/n)) for i in range(0,n+1,1)])
sine = arrayof([math.sin(math.pi * f * 2.0 * (i/n)) for i in range(0,n+1,1)])
#sine = arrayof([1.0,] + [0.0,] * n)

from timeit import default_timer as timer
from datetime import timedelta

Gain = 0.0
out = []
Amps = [Amp(gain=Gain,drive1=.89,drive2=.72,factor=4,stages=4,resonance=.625,blend=.707) for _ in range(2)]
start = timer()
for _ in range(4):
    for asig in sine[:-1]:
        t = asig
        for i,amp in enumerate(Amps):
            t = amp.process(t)
        out.append(t)
end = timer()
print(timedelta(seconds=end-start))

out2 = []
Amps2 = [Amp(gain=Gain,drive1=.89,drive2=.72,factor=4,stages=4,resonance=.625,blend=.707,iterations=1) for _ in range(2)]
start = timer()
for _ in range(4):
    for asig in sine[:-1]:
        t = asig
        for i,amp in enumerate(Amps2):
            t = amp.process(t)
        out2.append(t)
end = timer()
print(timedelta(seconds=end-start))

out3 = []
Amps3 = [Amp(gain=Gain,drive1=.89,drive2=.72,factor=4,stages=4,resonance=.625,blend=.707,iterations=1,internal_factor=4) for _ in range(2)]
start = timer()
for _ in range(4):
    for asig in sine[:-1]:
        t = asig
        for i,amp in enumerate(Amps3):
            t = amp.process(t)
        out3.append(t)
end = timer()
print(timedelta(seconds=end-start))

out6 = []
Amps6 = [Amp(gain=Gain,drive1=.89,drive2=.72,factor=4,stages=4,resonance=.625,blend=.707,iterations=1,internal_factor=6) for _ in range(2)]
start = timer()
for _ in range(4):
    for asig in sine[:-1]:
        t = asig
        for i,amp in enumerate(Amps6):
            t = amp.process(t)
        out6.append(t)
end = timer()
print(timedelta(seconds=end-start))

out7 = []
Amps7 = [Amp(gain=Gain,drive1=.89,drive2=.72,factor=4,stages=4,resonance=.625,blend=.707,iterations=32,internal_factor=4,use_lut=True) for _ in range(2)]
start = timer()
for _ in range(4):
    for asig in sine[:-1]:
        t = asig
        for i,amp in enumerate(Amps7):
            t = amp.process(t)
        out7.append(t)
end = timer()
print(timedelta(seconds=end-start))

plt.figure()
plt.plot(range(len(out)), out)
plt.plot(range(len(out2)), out2)
plt.plot(range(len(out3)), out3)
plt.plot(range(len(out6)), out6)
plt.plot(range(len(out7)), out7)
plt.show()
plt.figure()
F = numpy.fft.fft(out,4*n)
F = F[:len(F)//2]
plt.plot([i/4 for i in range(len(F))], numpy.abs(F)/n)
G = numpy.fft.fft(out2,4*n)
G = G[:len(G)//2]
plt.plot([i/4 for i in range(len(G))], numpy.abs(G)/n)
H = numpy.fft.fft(out3,4*n)
H = H[:len(H)//2]
plt.plot([i/4 for i in range(len(H))], numpy.abs(H)/n)
I = numpy.fft.fft(out6,4*n)
I = I[:len(I)//2]
plt.plot([i/4 for i in range(len(I))], numpy.abs(I)/n)
J = numpy.fft.fft(out7,4*n)
J = I[:len(J)//2]
plt.plot([i/4 for i in range(len(J))], numpy.abs(J)/n)
plt.show()
