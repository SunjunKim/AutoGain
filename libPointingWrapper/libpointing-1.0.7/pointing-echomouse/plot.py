#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# pointing-echomouse/plot.py --
#
# Initial software
# Authors: Nicolas Roussel
# Copyright © Inria

import os
import sys
import traceback
import math
import urllib.parse

from matplotlib import pyplot
from matplotlib.backends.backend_pdf import PdfPages

# --------------------------------------------------------------------------

def mm2in(d):
    return d/25.4

def in2m(d):
    return 0.0254*d

def parseConfigDict(path):
    d = {}
    with open(path) as fd:
        for line in fd:
            line = line.split('#', 1)[0].strip() # ignore everything that follows the first '#'
            if not line: continue
            key, value = map(lambda s: s.strip(), line.split(':', 1))
            d[key] = value
    return d

def exportPlot(fig, path, dpi=600):
    pdf = PdfPages(path)
    pdf.savefig(fig, dpi=dpi)
    pdf.close()

class Device(object):

    def __init__(self, uri):
        self.uri = uri
        purl = urllib.parse.urlparse(uri)
        pquery = urllib.parse.parse_qs(purl.query)
        for k in pquery.keys():
            v = pquery[k][0]
            try:
                v = float(v)
            except:
                pass
            setattr(self, k, v)

class PointingDevice(Device):
    pass

class DisplayDevice(Device):

    def __init__(self, uri):
        Device.__init__(self, uri)
        wInInches = mm2in(self.w)
        hInInches = mm2in(self.h)
        hPPI = self.bw / wInInches
        vPPI = self.bh / hInInches
        dInInches = math.sqrt(wInInches*wInInches + hInInches*hInInches)
        self.ppi = math.sqrt(self.bw*self.bw + self.bh*self.bh) / dInInches
    
class TransferFunction(object):

    def __init__(self, family, name, alias):
        self.family = family
        self.name = name
        self.alias = alias
        d = parseConfigDict(os.path.join(family.root, name+".dat"))
        self.raw_data = [float(d[str(i)]) for i in range(0, int(d["max-counts"])+1)]
        self.data = self.__cook()

    def __cook(self, ignorespecs=False):
        motor_speed, gain, visual_speed = [], [], []
        for counts, pixels in enumerate(self.raw_data):
            if ignorespecs:
                motor_speed.append(counts)
                gain.append(0.0 if counts==0.0 else pixels/counts)            
                visual_speed.append(pixels)
            else:
                ms = in2m(counts/self.family.pdev.cpi) * self.family.pdev.hz
                # The real speed that was "measured" on screen is
                #   vs = in2m(pixels/self.family.ddev.ppi) * self.family.ddev.hz
                # but we use a fixed ppi (110) and the input device's
                # frequency so that we can compare between families
                vs = in2m(pixels/110) * self.family.pdev.hz
                g = 0.0 if ms==0.0 else vs/ms
                motor_speed.append(ms)
                gain.append(g)            
                visual_speed.append(vs)
        return motor_speed, gain, visual_speed
        
class TransferFunctionFamily(object):

    def __init__(self, root):
        configfile = os.path.join(root, "config.dict")
        if not os.path.exists(configfile): raise ValueError(root)        
        self.root = root
        self.config = parseConfigDict(configfile)
        self.pdev = PointingDevice(self.config["libpointing-input"])
        self.ddev = DisplayDevice(self.config["libpointing-output"])
        self.functions = []
        self.function_by_alias = {}
        self.function_by_name = {}
        for name, alias in zip(self.config["functions"].split(','),
                               self.config["function-aliases"].split(',')):
            f = TransferFunction(self, name, alias)
            self.functions.append(f)
            self.function_by_name[name] = self.function_by_alias[alias] = f

    def plot(self, msmax=None, vsmax=None, gmax=None):
        fig, (ax1, ax2) = pyplot.subplots(ncols=2, figsize=(20, 10))
        fig.suptitle(self.config["system"])
        ax1.set_xlabel("Motor speed (m/s)")        
        ax1.set_ylabel("Gain")
        if msmax is not None: ax1.set_xlim([0, msmax])
        if gmax is not None: ax1.set_ylim([0, gmax])
        ax2.set_xlabel("Motor speed (m/s)")        
        ax2.set_ylabel("Visual speed (m/s)")
        if msmax is not None: ax2.set_xlim([0, msmax])
        if vsmax is not None: ax2.set_ylim([0, vsmax])
        for f in self.functions:
            motor_speed, gain, visual_speed = f.data           
            ax1.plot(motor_speed, gain, label=f.alias)
            ax2.plot(motor_speed, visual_speed, label=f.alias)
        pyplot.legend(loc="upper left", fancybox=True, framealpha=0.5, fontsize=10, bbox_to_anchor=(1.01,1.01))
        exportPlot(fig, os.path.join(self.root, "plots.pdf"))
        return fig

    @staticmethod
    def findAll(path):
        result = []
        for root, dirs, files in os.walk(path):
            try:
                result.append(TransferFunctionFamily(root))
            except ValueError:
                pass
            except:
                traceback.print_exc()
        return result
        
# --------------------------------------------------------------------------

families = [TransferFunctionFamily(path) for path in sys.argv[1:]]
if not families:
    families = TransferFunctionFamily.findAll(os.path.dirname(os.path.abspath(sys.argv[0])))

gmsmin = gvsmin = ggmin = float("inf")
gmsmax = gvsmax = ggmax = float("-inf")
for family in families:
    print(os.path.relpath(family.root))
    print()
    print("   %s (%d functions)"%(family.config["system"], len(family.functions)))
    print("  ", family.pdev.uri)
    print("  ", family.ddev.uri)
    print("  ", family.ddev.ppi)
    print()
    for f in family.functions:
        print("   %s (%s)"%(f.name, f.alias))
        motor_speed, gain, visual_speed = f.data
        msmin, msmax = min(motor_speed), max(motor_speed)
        vsmin, vsmax = min(visual_speed), max(visual_speed)
        gmin, gmax = min(gain), max(gain)
        print("      Motor speed is in [%s, %s]"%(msmin, msmax))
        print("      Visual speed is in [%s, %s]"%(vsmin, vsmax))
        print("      Gain is in [%s, %s]"%(gmin, gmax))
        gmsmin = min(gmsmin, msmin)
        gmsmax = max(gmsmax, msmax)
        gvsmin = min(gvsmin, vsmin)
        gvsmax = max(gvsmax, vsmax)
        ggmin = min(ggmin, gmin)
        ggmax = max(ggmax, gmax)
    print()
print("Motor speed is in [%s, %s]"%(gmsmin, gmsmax))
print("Visual speed is in [%s, %s]"%(gvsmin, gvsmax))
print("Gain is in [%s, %s]"%(ggmin, ggmax))

for family in families:
    # fig = family.plot(gmsmax, gvsmax, ggmax)
    fig = family.plot()
    
# pyplot.show()
