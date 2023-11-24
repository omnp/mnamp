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

n = 48000#4096
sr = float(n)
f = 50.0
sine = array.array('f', ([math.sin(math.pi * f * 2.0 * (i/n)) for i in range(0,n+1,1)]))
out = array.array('f', [0.0,]*(n+1))
amp = descriptor.instantiate(ffi.cast('LV2_Descriptor *', descriptor), sr, ffi.cast('const char *', 0), ffi.cast('const LV2_Feature * const*', 0))
buffer_length = 32
ports = {
        'out':ffi.new('float [{}]'.format(buffer_length)),
        'in':ffi.new('float [{}]'.format(buffer_length)),
        'gain1':ffi.new('float *'),
        'gain2':ffi.new('float *'),
        'toggle':ffi.new('float *'),
        'cutoff':ffi.new('float *'),
        'volume':ffi.new('float *'),
        'stages':ffi.new('float *'),
        'compensation':ffi.new('float *'),
        'drive1':ffi.new('float *'),
        'drive2':ffi.new('float *'),
        'resonance':ffi.new('float *'),
        'factor':ffi.new('float *'),
        'eps':ffi.new('float *'),
        'tension':ffi.new('float *'),
        'eq':ffi.new('float *'),
        }
print(ports)
descriptor.connect_port(amp, 0, ports['out'])
descriptor.connect_port(amp, 1, ports['in'])
descriptor.connect_port(amp, 2, ports['gain1'])
descriptor.connect_port(amp, 3, ports['gain2'])
descriptor.connect_port(amp, 4, ports['toggle'])
descriptor.connect_port(amp, 5, ports['cutoff'])
descriptor.connect_port(amp, 6, ports['stages'])
descriptor.connect_port(amp, 7, ports['drive1'])
descriptor.connect_port(amp, 8, ports['drive2'])
descriptor.connect_port(amp, 9, ports['resonance'])
descriptor.connect_port(amp, 10, ports['factor'])
descriptor.connect_port(amp, 11, ports['eps'])
descriptor.connect_port(amp, 12, ports['tension'])
descriptor.connect_port(amp, 133, ports['eq'])
descriptor.connect_port(amp, 14, ports['compensation'])
descriptor.connect_port(amp, 15, ports['volume'])
descriptor.activate(amp)
ports['gain1'][0] = ffi.cast('float', 24.0)
ports['gain2'][0] = ffi.cast('float', 48.0)
ports['toggle'][0] = ffi.cast('float', 0.0)
ports['cutoff'][0] = ffi.cast('float', 7200.0)
ports['volume'][0] = ffi.cast('float', -0.0)
ports['stages'][0] = ffi.cast('float', ffi.cast('unsigned int', 12))
ports['compensation'][0] = ffi.cast('float', -0.0)
ports['drive1'][0] = ffi.cast('float', 0.707)
ports['drive2'][0] = ffi.cast('float', 0.606)
ports['resonance'][0] = ffi.cast('float', 0.78)
ports['factor'][0] = ffi.cast('float', ffi.cast('unsigned int', 4))
ports['eps'][0] = ffi.cast('float', 1.0)
ports['tension'][0] = ffi.cast('float', 72000.0)
ports['eq'][0] = ffi.cast('float', 1.0)
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
F = numpy.fft.fft(out,n)
F = F[:len(F)//2]
plot.plot([i for i in range(len(F))], numpy.abs(F)/n)
plot.show()

