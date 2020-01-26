#include <dependency_graph/rtti.h>
#include <dependency_graph/data.inl>

#include <actions/io.h>
#include <actions/node_data.h>

#include "io.h"

namespace dependency_graph { namespace io {

struct SaveableRegistration {
	SaveableRegistration() {
		setIsSaveableCallback([](const BaseData&) {
			return true;
		});
	}

	~SaveableRegistration() {
		setIsSaveableCallback(std::function<bool(const BaseData& data)>());
	}
};

static SaveableRegistration s_saveableRegistration;

} }

namespace possumwood { namespace io {

void fromJson(const json& j, dependency_graph::BaseData& data) {
	if(data.type() == "float") {
		dependency_graph::TypedData<float>& typed = dynamic_cast<dependency_graph::TypedData<float>&>(data);
		typed.set(j.get<float>());
	}
	else if(data.type() == dependency_graph::unmangledTypeId<std::string>()) {
		dependency_graph::TypedData<std::string>& typed = dynamic_cast<dependency_graph::TypedData<std::string>&>(data);
		typed.set(j.get<std::string>());
	}
	else if(data.type() == dependency_graph::unmangledTypeId<possumwood::NodeData>()) {
		dependency_graph::TypedData<possumwood::NodeData>& typed = dynamic_cast<dependency_graph::TypedData<possumwood::NodeData>&>(data);
		typed.set(possumwood::NodeData());
	}
	else
		assert(false);
}

void toJson(json& j, const dependency_graph::BaseData& data) {
	if(data.type() == "float") {
		const dependency_graph::TypedData<float>& typed = dynamic_cast<const dependency_graph::TypedData<float>&>(data);
		j = typed.get();
	}
	else if(data.type() == dependency_graph::unmangledTypeId<std::string>()) {
		const dependency_graph::TypedData<std::string>& typed = dynamic_cast<const dependency_graph::TypedData<std::string>&>(data);
		j = typed.get();
	}
	else if(data.type() == dependency_graph::unmangledTypeId<possumwood::NodeData>()) {
		// const dependency_graph::TypedData<possumwood::NodeData>& typed = dynamic_cast<const dependency_graph::TypedData<possumwood::NodeData>&>(data);
		j = "test blind data";
	}
	else
		assert(false);
}

bool isSaveable(const dependency_graph::BaseData& data) {
	return true;
}

} }
