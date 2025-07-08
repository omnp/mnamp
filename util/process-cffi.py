import cffi

ffi = cffi.FFI()
ffi.cdef(
r"""
typedef void* LV2_Handle;
typedef struct {
  const char* URI;
  void* data;
} LV2_Feature;

typedef struct LV2_Descriptor {
  const char* URI;
  LV2_Handle (*instantiate)(const struct LV2_Descriptor* descriptor,
                            double                       sample_rate,
                            const char*                  bundle_path,
                            const LV2_Feature* const*    features);
  void (*connect_port)(LV2_Handle instance, uint32_t port, void* data_location);
  void (*activate)(LV2_Handle instance);
  void (*run)(LV2_Handle instance, uint32_t sample_count);
  void (*deactivate)(LV2_Handle instance);
  void (*cleanup)(LV2_Handle instance);
  const void* (*extension_data)(const char* uri);
} LV2_Descriptor;
""" +\
r"""
const LV2_Descriptor *lv2_descriptor (const uint32_t index);
""")
lib = ffi.dlopen('../build/mnamp.lv2/mnamp.so')
descriptor = lib.lv2_descriptor(0)
print(descriptor)
print(dir(descriptor))

import math
import array
import numpy
import matplotlib.pyplot as plot

n = 48000
sr = float(n)
f = 110.0
sine = numpy.array(([0.0 for _ in range(n)] + [math.sin(math.pi * f * 2.0 * (i/n)) for i in range(0,n+1,1)]))
out = array.array('f', [0.0,]*(2*n+1))
amp = descriptor.instantiate(ffi.cast('LV2_Descriptor *', descriptor), sr, ffi.cast('const char *', 0), ffi.cast('const LV2_Feature * const*', 0))
buffer_length = 32
ports = {
        'out':ffi.new('float [{}]'.format(buffer_length)),
        'in':ffi.new('float [{}]'.format(buffer_length)),
        'cutoff':ffi.new('float *'),
        'stages':ffi.new('float *'),
        'resonance':ffi.new('float *'),
        'eps':ffi.new('float *'),
        'eq':ffi.new('float *'),
        'compensation':ffi.new('float *'),
        'volume':ffi.new('float *'),
        'gain':ffi.new('float *'),
        }
print(ports)
descriptor.connect_port(amp, 0, ports['out'])
descriptor.connect_port(amp, 1, ports['in'])
descriptor.connect_port(amp, 2, ports['cutoff'])
descriptor.connect_port(amp, 3, ports['stages'])
descriptor.connect_port(amp, 4, ports['resonance'])
descriptor.connect_port(amp, 5, ports['eps'])
descriptor.connect_port(amp, 6, ports['eq'])
descriptor.connect_port(amp, 7, ports['compensation'])
descriptor.connect_port(amp, 8, ports['volume'])
descriptor.connect_port(amp, 9, ports['gain'])
descriptor.activate(amp)
ports['gain'][0] = ffi.cast('float', 23.959)
ports['cutoff'][0] = ffi.cast('float', 1994.0)
ports['stages'][0] = ffi.cast('float', ffi.cast('unsigned int', 16))
ports['resonance'][0] = ffi.cast('float', 1.707)
ports['eps'][0] = ffi.cast('float', 0.707)
ports['eq'][0] = ffi.cast('float', 0.509)
ports['compensation'][0] = ffi.cast('float', -0.5)
ports['volume'][0] = ffi.cast('float', -0.0)
for i in range(0,len(sine)-1,buffer_length):
    for j in range(0, buffer_length):
        ports['in'][j] = sine[i+j]
    descriptor.run(amp, buffer_length)
    for j in range(0, buffer_length):
        out[i+j] = ports['out'][j]
descriptor.deactivate(amp)
descriptor.cleanup(amp)
plot.figure()
plot.plot(range(len(sine)), sine)
plot.plot(range(len(out)), out)
plot.show()
F = numpy.fft.fft(out[n:],n)
F = F[:len(F)//2]
plot.figure()
plot.plot([i for i in range(len(F))], numpy.abs(F)/n)
plot.show()
