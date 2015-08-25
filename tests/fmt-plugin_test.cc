#include "hogl/format-basic.hpp"
#include "hogl/plugin/format.hpp"

// Basic test for the loadable format plugin

// Custom format handler
class custom_format : public hogl::format_basic {
public:
	custom_format() :
		hogl::format_basic("timespec|ring|seqnum|area|section")
	{}
};

// Allocate and initialize format plugin.
// Returns a pointer to initialized hogl::format instance
static hogl::format* create(const char *str)
{
	custom_format *fmt = new custom_format();
	return fmt;
}

// Release all memmory allocated by format plugin.
static void release(hogl::format *f)
{
	custom_format *fmt = reinterpret_cast<custom_format *>(f);
	delete fmt;
}

// Each plugin must export this symbol
hogl::plugin::format __hogl_plugin_format = { create, release };
