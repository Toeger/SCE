#ifndef PROTOBUFFER_ENCODER_H
#define PROTOBUFFER_ENCODER_H

#include <sce.pb.h>
#include <string>

namespace Utility {
	//TODO: Instead of a template this could be a function that takes a google::protobuf::Message &. It's virtual dispatch vs code size. No idea which is more
	//      important.
	template <class Pb_message>
	std::string encode(const Pb_message &pb_message, std::string &buffer) {
		static_assert(std::is_base_of_v<google::protobuf::Message, Pb_message>, "Encoded object must be a protobuffer message");
		assert(pb_message.IsInitialized());
		buffer = pb_message.GetDescriptor()->name();
		buffer += ':';
		buffer += std::to_string(pb_message.ByteSize());
		buffer += ':';
		pb_message.AppendToString(&buffer);
		return buffer;
	}
	template <class Pb_message>
	std::string encode(const Pb_message &pb_message) {
		std::string buffer;
		encode(pb_message, buffer);
		return buffer;
	}
} // namespace Utility

#endif