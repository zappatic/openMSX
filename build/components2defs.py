# $Id$
# Generates the contents of "components_defs.mk".

from components import checkComponents, coreLibs
from makeutils import extractMakeVariables
from outpututils import rewriteIfChanged

import sys

def iterComponentDefs(probeMakePath):
	probeVars = extractMakeVariables(probeMakePath)
	buildCore, buildGL, buildJack = checkComponents(probeVars)

	yield '# Automatically generated by build process.'
	yield 'CORE_LIBS:=%s' % ' '.join(coreLibs)
	yield 'COMPONENT_CORE:=%s' % str(buildCore).lower()
	yield 'COMPONENT_GL:=%s' % str(buildGL).lower()
	yield 'COMPONENT_JACK:=%s' % str(buildJack).lower()

if len(sys.argv) == 3:
	rewriteIfChanged(sys.argv[1], iterComponentDefs(sys.argv[2]))
else:
	print >>sys.stderr, (
		'Usage: python components2defs.py COMPONENTS_DEFS PROBE_MAKE'
		)
	sys.exit(2)
