import math
import array
import numpy
import matplotlib.pyplot as plot

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
        self.d = 1.0 + self.k/q + self.k*self.k
        
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
    
    def reset(self):
        self.__init__()

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
    def __init__(self, gain=1., drive1=1., drive2=1., factor=2, stages=1, compensation=0., resonance=1., blend=1., iterations=16, tension=1e-6, use_lut=False, max_factor=16) -> None:
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
        self.tension = tension
        self.use_lut = use_lut
        if self.use_lut:
            self.umax = 384.
            self.ustep = 16
            self.gmax = 2.
            self.gstep = 8
            self.tmax = 2.
            self.tstep = 2
            self.lut = zeros((self.tstep * self.gstep * self.ustep,))
            for t in range(0,self.tstep):
                for g in range(0,self.gstep):
                    for u in range(0, self.ustep):
                        self.tension = t / self.tstep * self.tmax
                        x = self.fc(u / self.ustep * self.umax, g / self.gstep * self.gmax)
                        self.lut[t*self.gstep*self.ustep+g*self.ustep+u] = x
            self.fc = self.fc_lut
        self.max_factor = max_factor
    
    def fc_lut(self, u, g):
        tn = self.tension
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
    
    def h(self, x, limit):
        s = x > 0 - x < 0
        return s * min(abs(x), limit)
        
    def fc(self, u, g):
        tension = self.tension
        k = 0
        fl = BQFilter(8)
        Q = 0.707
        cutoff = 0.5 / self.factor
        fl.setparams(cutoff, Q)
        f = 1.0
        while k < self.iterations:
            s = 0.0
            r = 0.0
            fl.reset()
            for i in range(0,self.factor):
                y = self.f(u[i], 1.)
                fl.process(y)
                r += abs(y)
                s += abs(fl.lp)
            k += 1
            if abs(r - s) > tension:
                if f * (1. - 1. / (1 << k)) >= 0.0:
                    f -=  1. / (1 << k)
            else:
                break
        return f * g
    
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
            self.gains[h].setparams(k = 50.0 / sr, q = .707)
        
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
            buffer[0] = self.filters[h][0].process(t * sampling)
            for j in range(1,sampling):
                buffer[j] = self.filters[h][0].process(0.0)
            
            self.gains[h].process(1.0)
            g = self.fc(buffer, abs(self.gains[h].lp))
            self.gains[h].process(g)
            g = abs(self.gains[h].lp)
            for j in range(sampling):
                t = buffer[j]
                t = self.f(g * t, 1.)
                buffer[j] = t

            for j in range(sampling):
                t = buffer[j]
                t = self.filters[h][1].process(t)
            
            # polynomial shaping
            buffer[0] = self.filters[h][2].process(t * sampling4x)
            for j in range(1,sampling4x):
                buffer[j] = self.filters[h][2].process(0.0)
            for j in range(sampling4x):
                buffer[j] = nonlin(buffer[j] * sampling4x, h)
            for j in range(sampling4x):
                t = self.filters[h][3].process(buffer[j])
            # end polynomial shaping

            self.highpass[h].process(t)
            t = self.highpass[h].hp
            t = t * dbl(self.compensation)
        
        return t

class Oscil:
    def __init__(self, sr, table) -> None:
        self.sr = sr
        self.table = table
        self.phasor = 0.0
    
    def process(self, frequency):
        return self.read(frequency)

    def read(self, frequency, phase=0.0):
        size = len(self.table)
        self.phasor += frequency / self.sr
        while self.phasor >= 1.0:
            self.phasor -= 1.0
        while self.phasor < 0.0:
            self.phasor += 1.0
        phase += self.phasor
        while phase >= 1.0:
            phase -= 1.0
        while phase < 0.0:
            phase += 1.0
        iphase = int(size * phase)
        frac = size * phase - float(iphase)
        return frac * self.table[(iphase + 1) % size] + (1. - frac) * self.table[(iphase) % size]

class Synth:
    def __init__(self, sr, carrier, factor, stages, iterations, tension = 1e-6, feedback = 0.0) -> None:
        self.sr = sr
        self.carrier = carrier
        self.phasor = 0.0
        self.stages = stages
        self.filters = [[BQFilter(8) for _ in range(8)] for _ in range(self.stages)]
        self.highpass = [Filter() for _ in range(self.stages)]
        self.gains = [[BQFilter(8) for _ in range(2)] for _ in range(self.stages)]
        self.max_factor = 16
        self.iterations = iterations
        self.resonance = 0.707
        self.buffer = BQFilter(6)
        self.buffer.setparams(0.25, 1.0)
        self.feedback = feedback
        self.tension = tension
        self.factor = factor
    
    def bisect(self, fn, t, u, g, reset_state=lambda: None):
        k = 0
        hgh = BQFilter(8)
        Q = 0.707
        hgh.setparams(0.5 / self.factor, Q)
        f = 1.0
        while k < self.iterations:
            reset_state()
            s = 0.0
            r = 0.0
            hgh.reset()
            for i in range(0,self.factor):
                y = fn(t[i], u[i] * f * g)
                r += abs(y)
                hgh.process(y)
                s += abs(hgh.lp)
            k += 1
            if abs(r - s) > self.tension:
                if f * (1. - 1. / (1 << k)) >= 0.0:
                    f -= 1. / (1 << k)
            else:
                break
        return f * g
     
    def process(self, frequency:float, am_modulator = lambda: 0., pm_modulator = lambda: 0.) -> float:
        sampling = self.factor
        buffers = {name: zeros((self.max_factor,)) for name in ['carrier', 'am', 'fm', 'gains']}

        # Set params
        for h in range(self.stages):
            for i in range(0,8):
                self.filters[h][i].setparams(0.5 / sampling, self.resonance)
            self.highpass[h].setparams(.0001 * 48000.0 / self.sr,0.9,1.0)
            for i in range(2):
                self.gains[h][i].setparams(k = 50.0 / sr, q = .707)
        
        # Modulation
        phasor = 0.0
        def fm_store():
            nonlocal phasor
            phasor = self.phasor
        def fm_reset():
            nonlocal phasor
            self.phasor = phasor
        def fm(v):
            nonlocal frequency
            y = self.read(self.carrier, frequency, v)
            return y
        am = lambda t,u: (t * (1. + u))
        u = 0.25 * self.feedback * self.buffer.lp + am_modulator() * 0.25
        v = 0.125 * self.feedback * self.buffer.lp + pm_modulator() * 0.125
        #u = am_modulator()
        #v = pm_modulator() * 0.125
        
        for h in range(self.stages):
            # Upsample modulator
            buffers['fm'][0] = self.filters[h][1].process(v * sampling)
            for j in range(1,sampling):
                buffers['fm'][j] = self.filters[h][1].process(0.0)
            # Process
            self.gains[h][0].process(1.0)
            f = self.gains[h][0].lp
            fm_store()
            f = self.bisect(lambda _,v: fm(v), buffers['fm'], buffers['fm'], f, reset_state=fm_reset)
            fm_reset()
            self.gains[h][0].process(f)
            f = self.gains[h][0].lp
            for j in range(sampling):
                v = buffers['fm'][j]
                t = fm(f * v)
                buffers['carrier'][j] = t
            # Upsample modulator
            buffers['am'][0] = self.filters[h][5].process(u * sampling)
            for j in range(1,sampling):
                buffers['am'][j] = self.filters[h][5].process(0.0)
            # Process
            self.gains[h][1].process(1.0)
            g = self.gains[h][1].lp
            g = self.bisect(am, buffers['carrier'], buffers['am'], g)
            self.gains[h][1].process(g)
            g = self.gains[h][1].lp
            for j in range(sampling):
                t_ = buffers['carrier'][j]
                u_ = buffers['am'][j]
                t = am(t_, g * u_)
                buffers['carrier'][j] = t
            # Downsample
            for j in range(sampling):
                t = buffers['carrier'][j]
                t = self.filters[h][6].process(t)
            # Filter
            self.highpass[h].process(t)
            t = self.highpass[h].hp
        
        self.buffer.process(t)
        return t

    def read(self, table, frequency, phase=0.0):
        size = len(table)
        while self.phasor >= 1.0:
            self.phasor -= 1.0
        while self.phasor < 0.0:
            self.phasor += 1.0
        phase += self.phasor
        while phase >= 1.0:
            phase -= 1.0
        while phase < 0.0:
            phase += 1.0
        iphase = int(size * phase)
        frac = size * phase - float(iphase)
        self.phasor += frequency / self.sr
        return frac * table[(iphase + 1) % size] + (1. - frac) * table[(iphase) % size]

n = 48000
sr = float(n)
f = 1.
sine = arrayof([math.sin(math.pi * f * 2.0 * (i/n)) for i in range(0,n+1,1)])

from timeit import default_timer as timer
from datetime import timedelta

out = []
synth = Synth(sr, sine, factor=2, stages=2, iterations=36, tension=1e-10, feedback=0.45)
amp = Amp(1.5, drive1=0.666, drive2=0.777, factor=2, stages=1, compensation=0.0, resonance=0.707, blend=1.0, iterations=36, tension=1e-10)
am_modulator = Oscil(sr, sine)
pm_modulator = Oscil(sr, sine)
frequency0 = 33.0
frequency1 = sr / 2.0 - 1.0
frequency2 = sr / 8.0
frequency3 = 11.0
am = lambda: am_modulator.process(25.0)#0.25)
pm = lambda: pm_modulator.process(50.0)#0.5)
filter = BQFilter(8)
filter.setparams(1200.0 / sr, 0.707)
start = timer()
k = 2#5#
for i in range(k*n):
    synth.feedback = 0.125*float(i)/float(n*k)
    frequency = frequency0 - (frequency0 - frequency3) * (i/(k*n))
    t = synth.process(frequency,am,pm)
    cutoff = (1200.0 - 800.0 * float(i)/float(n*k))
    q = 0.707 - 0.444 * float(i)/float(n*k)
    filter.setparams(cutoff / sr, q)
    filter.process(t)
    t = amp.process(filter.lp)
    out.append(t)
end = timer()
print(timedelta(seconds=end-start))

plot.figure()
plot.plot(range(len(out)), out)
plot.show()
plot.figure()
F = numpy.fft.fft(out,k*n)
F = F[:len(F)//2]
plot.plot([i/k for i in range(len(F))], numpy.abs(F)/n)
plot.show()
out_ = array.array('f', out)

import sys
import wave
if len(sys.argv) > 1:
    f = wave.open(sys.argv[1], 'wb')
    f.setnchannels(1)
    f.setsampwidth(4)
    f.setframerate(sr)
    f.writeframes(out_)
    f.close()
