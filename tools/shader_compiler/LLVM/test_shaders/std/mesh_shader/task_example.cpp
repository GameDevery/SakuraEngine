#include "attributes.hpp"
#include <skr/std.hpp>
using namespace skr::shader;
struct VertexIn {
	[[INSTANCE_ID]] uint instance_id;
	[[MESHLET_ID]] uint meshlet_id;
};
[[TASK_SHADER]] bool IsVisible(
	VertexIn vertex_in) {
    // Draw every mesh
	return true;
}