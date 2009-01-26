# $Id$

from makeutils import extractMakeVariables, parseBool

import sys

def iterBuildInfoHeader(targetPlatform, targetCPU, flavour, installShareDir):
	cpuVars = extractMakeVariables('build/cpu-%s.mk' % targetCPU)
	bigEndian = parseBool(cpuVars['BIG_ENDIAN'])
	unalignedMemoryAccess = parseBool(
		cpuVars.get('UNALIGNED_MEMORY_ACCESS', 'false')
		)

	platformVars = extractMakeVariables('build/platform-%s.mk' % targetPlatform)
	setWindowIcon = parseBool(platformVars.get('SET_WINDOW_ICON', 'true'))

	# TODO: Add support for device-specific configuration.
	platformGP2X = False
	if platformGP2X:
		have16BPP = True
		have32BPP = False
		minScaleFactor = 1
		maxScaleFactor = 1
	else:
		have16BPP = True
		have32BPP = True
		minScaleFactor = 1
		maxScaleFactor = 4

	yield '// Automatically generated by build process.'
	yield ''
	yield '#ifndef BUILD_INFO_HH'
	yield '#define BUILD_INFO_HH'
	yield ''
	yield '#include <string>'
	yield ''
	# Use a macro iso a boolean to prevent compilation errors on non-x86 machines.
	if targetCPU == 'x86':
		yield '#define ASM_X86_32'
		yield '#define ASM_X86'
	elif targetCPU == 'x86_64':
		yield '#define ASM_X86_64'
		yield '#define ASM_X86'
	# Use a macro iso integer because we really need to exclude code sections
	# based on this.
	yield '#define PLATFORM_GP2X %d' % platformGP2X
	yield '#define HAVE_16BPP %d' % have16BPP
	yield '#define HAVE_32BPP %d' % have32BPP
	yield '#define MIN_SCALE_FACTOR %d' % minScaleFactor
	yield '#define MAX_SCALE_FACTOR %d' % maxScaleFactor
	yield ''
	yield 'namespace openmsx {'
	yield ''
	# Note: Don't call it "BIG_ENDIAN", because some system header may #define
	#       that.
	yield 'static const bool OPENMSX_BIGENDIAN = %s;' % str(bigEndian).lower()
	yield 'static const bool OPENMSX_UNALIGNED_MEMORY_ACCESS = %s;' \
		% str(unalignedMemoryAccess).lower()
	yield 'static const bool OPENMSX_SET_WINDOW_ICON = %s;' \
		% str(setWindowIcon).lower()
	yield 'static const std::string DATADIR = "%s";' % installShareDir
	yield 'static const std::string BUILD_FLAVOUR = "%s";' % flavour
	yield ''
	yield '} // namespace openmsx'
	yield ''
	yield '#endif // BUILD_INFO_HH'

if len(sys.argv) == 5:
	for line in iterBuildInfoHeader(*sys.argv[1 : ]):
		print line
else:
	print >>sys.stderr, \
		'Usage: python buildinfo2code.py platform cpu flavour share-install-dir'
	sys.exit(2)
