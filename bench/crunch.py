#!/usr/bin/env python

import sys
import json
from numpy import array

def load_results(kind):
	results = []
	for i in range(0, 10):
		f = open('results/%s.%i' % (kind, i), 'r')
		s = f.read()
		results.append(json.loads(s)['elapsed_usec'])
	return results

if __name__ == '__main__':
	celt = array(load_results('celt'))
	sbcelt = array(load_results('sbcelt'))

	print '# results (niter=1000)'
	print 'sbcelt  %.2f usec (%.2f stddev)' % (sbcelt.mean(), sbcelt.std())
	print 'celt    %.2f usec (%.2f stddev)' % (celt.mean(), celt.std())
	print 'sbcelt delta: %.2f usec' % (celt.mean() - sbcelt.mean())
