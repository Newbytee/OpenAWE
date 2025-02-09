/* OpenAWE - A reimplementation of Remedys Alan Wake Engine
 *
 * OpenAWE is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * OpenAWE is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenAWE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenAWE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdexcept>
#include <iostream>
#include <vector>
#include <map>

#include <glm/glm.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "havokfile.h"

namespace AWE {

struct Section {
	std::string sectionName;
	uint32_t absoluteDataStart;
	uint32_t localFixupsOffset;
	uint32_t globalFixupsOffset;
	uint32_t virtualFixupsOffset;
	uint32_t exportsOffset;
	uint32_t importsOffset;
	uint32_t endOffset;
};

enum QuantizationType {
	k8Bit = 0,
	k16Bit,
	k32Bit,
	k40Bit,
	k48Bit
};

struct TransformMask {
	uint8_t quantizationTypes;
	uint8_t positionTypes;
	uint8_t rotationTypes;
	uint8_t scaleTypes;

	QuantizationType transformType;
};

HavokFile::HavokFile(Common::ReadStream &binhkx) {
	const uint32_t magicId1 = binhkx.readUint32LE();
	const uint32_t magicId2 = binhkx.readUint32LE();

	if (magicId1 != 0x57E0E057 || magicId2 != 0x10C0C010)
		throw std::runtime_error("Invalid magic id");

	const uint32_t userTag = binhkx.readUint32LE();
	const uint32_t fileVersion = binhkx.readUint32LE();

	binhkx.skip(4);

	const uint32_t numSections = binhkx.readUint32LE();

	const uint32_t contentsSectionIndex = binhkx.readUint32LE();
	const uint32_t contentsSectionOffset = binhkx.readUint32LE();

	const uint32_t classNameSectionIndex = binhkx.readUint32LE();
	const uint32_t classNameSectionOffset = binhkx.readUint32LE();

	std::string versionName = binhkx.readFixedSizeString(15, true);
	setHeader(versionName);
	binhkx.skip(1);

	const uint32_t flags = binhkx.readUint32LE();

	binhkx.skip(4);

	std::vector<Section> sections;
	sections.resize(numSections);
	for (auto &section : sections) {
		section.sectionName = binhkx.readFixedSizeString(19, true);

		binhkx.skip(1);

		section.absoluteDataStart = binhkx.readUint32LE();
		section.localFixupsOffset = binhkx.readUint32LE();
		section.globalFixupsOffset = binhkx.readUint32LE();
		section.virtualFixupsOffset = binhkx.readUint32LE();
		section.exportsOffset = binhkx.readUint32LE();
		section.importsOffset = binhkx.readUint32LE();
		section.endOffset = binhkx.readUint32LE();

		_sectionOffsets.emplace_back(section.absoluteDataStart);
	}

	const Section &classNameSection = sections[classNameSectionIndex];
	const Section &contentsSection = sections[contentsSectionIndex];

	std::map<uint32_t, std::string> classNames;

	binhkx.seek(contentsSection.absoluteDataStart);
	std::unique_ptr<Common::ReadStream> havokContent(binhkx.readStream(contentsSection.endOffset - contentsSection.absoluteDataStart));

	binhkx.seek(classNameSection.absoluteDataStart);
	while (true) {
		const uint32_t tag = binhkx.readUint32LE();
		if ((tag & 0xFFu) == 255)
			break;

		binhkx.skip(1);

		const uint32_t position = binhkx.pos() - classNameSection.absoluteDataStart;
		const std::string className = binhkx.readNullTerminatedString();

		classNames[position] = className;
	}

	binhkx.seek(contentsSection.absoluteDataStart + contentsSection.localFixupsOffset);
	while (true) {
		uint32_t address = binhkx.readUint32LE();
		uint32_t targetAddress = binhkx.readUint32LE();

		if (targetAddress == 0xFFFFFFFF || binhkx.eos() || binhkx.pos() - contentsSection.absoluteDataStart > contentsSection.globalFixupsOffset)
			break;

		Fixup fixup;
		/*if (_version == kHavok2010_2_0_r1)
			fixup.targetAddress = targetAddress + _sectionOffsets[contentsSectionIndex];
		else*/
			fixup.targetAddress = targetAddress;
		fixup.section = contentsSectionIndex;
		_fixups[address] = fixup;
		//fmt::print("Local Fixup {} {}\n", address, targetAddress);
	}

	binhkx.seek(contentsSection.absoluteDataStart + contentsSection.globalFixupsOffset);
	while (true) {
		uint32_t address = binhkx.readUint32LE();
		if (address == 0xFFFFFFFF || binhkx.eos() || binhkx.pos() - contentsSection.absoluteDataStart > contentsSection.virtualFixupsOffset)
			break;

		uint32_t section = binhkx.readUint32LE();
		uint32_t metaAddress = binhkx.readUint32LE();

		Fixup fixup;
		fixup.targetAddress = metaAddress;
		fixup.section = section;
		_fixups[address] = fixup;

		//fmt::print("Global Fixup {} {} {}\n", address, metaAddress, section);
	}

	binhkx.seek(contentsSection.absoluteDataStart + contentsSection.virtualFixupsOffset);
	while (!binhkx.eos()) {
		uint32_t address = binhkx.readUint32LE();
		uint32_t section = binhkx.readUint32LE();

		if (address == 0xFFFFFFFF)
			break;

		uint32_t nameAddress = binhkx.readUint32LE();
		std::string name = classNames[nameAddress];

		//std::cout << name << std::endl;
		//fmt::print("Address {}\n", contentsSection.absoluteDataStart + address);

		size_t lastPos = binhkx.pos();
		binhkx.seek(contentsSection.absoluteDataStart + address);
		havokContent->seek(address);

		std::any object;
		if (name == "hkaSkeleton")
			object = readHkaSkeleton(binhkx, contentsSectionIndex);
		else if (name == "hkRootLevelContainer")
			readHkRootLevelContainer(*havokContent);
		else if (name == "hkaSplineCompressedAnimation")
			object = readHkaSplineCompressedAnimation(binhkx, contentsSectionIndex);
		else if (name == "hkaAnimationBinding")
			readHkaAnimationBinding(binhkx, contentsSectionIndex);
		else if (name == "hkaAnimationContainer")
			readHkaAnimationContainer(binhkx, contentsSectionIndex);
		else if (name == "hkxScene")
			readHkxScene(binhkx);
		else if (name == "RmdPhysicsSystem")
			_physicsSystem = readRmdPhysicsSystem(binhkx, contentsSectionIndex);
		else if (name == "hkpRigidBody")
			object = readHkpRigidBody(binhkx, contentsSectionIndex);
		else if (name == "hkpBoxShape")
			object = readHkpBoxShape(binhkx);
		else
			spdlog::warn("TODO: Implement havok class {}", name);

		if (object.has_value())
			_objects[contentsSection.absoluteDataStart + address] = object;
		binhkx.seek(lastPos);
	}
}

const HavokFile::hkaAnimationContainer& HavokFile::getAnimationContainer() const {
	return _animationContainer;
}

const HavokFile::RmdPhysicsSystem& HavokFile::getPhysicsSystem() const {
	return _physicsSystem;
}

HavokFile::hkaSkeleton HavokFile::getSkeleton(uint32_t address) {
	auto skeleton = std::any_cast<hkaSkeleton>(_objects[address]);
	return skeleton;
}

HavokFile::hkaAnimation HavokFile::getAnimation(uint32_t address) {
	auto animation = std::any_cast<hkaAnimation>(_objects[address]);
	return animation;
}

HavokFile::hkpRigidBody HavokFile::getRigidBody(uint32_t address) {
	auto rigidBody = std::any_cast<hkpRigidBody>(_objects[address]);
	return rigidBody;
}

HavokFile::hkpShape HavokFile::getShape(uint32_t address) {
	auto shape = std::any_cast<hkpShape>(_objects[address]);
	return shape;
}

std::vector<uint32_t> HavokFile::readUint32Array(Common::ReadStream &binhkx, HavokFile::hkArray array) {
	std::vector<uint32_t> data(array.count);

	if (array.offset == 0xFFFFFFFF)
		return data;

	binhkx.seek(array.offset);
	for (auto &date : data) {
		date = binhkx.readUint32LE();
	}

	return data;
}

std::vector<int16_t> HavokFile::readSint16Array(Common::ReadStream &binhkx, HavokFile::hkArray array) {
	std::vector<int16_t> data(array.count);

	if (array.offset == 0xFFFFFFFF)
		return data;

	binhkx.seek(array.offset);
	for (auto &date : data) {
		date = binhkx.readUint32LE();
	}

	return data;
}

std::vector<uint32_t>
HavokFile::readFixupArray(Common::ReadStream &binhkx, HavokFile::hkArray array, uint32_t section) {
	std::vector<uint32_t> data(array.count);

	if (array.offset == 0xFFFFFFFF)
		return data;

	binhkx.seek(array.offset);
	for (auto &date : data) {
		date = readFixup(binhkx, section);
	}

	return data;
}

std::vector<uint32_t>
HavokFile::readFixupArray(Common::ReadStream &binhkx, uint32_t offset, uint32_t count, uint32_t section) {
	std::vector<uint32_t> data(count);

	binhkx.seek(offset);
	for (auto &date : data) {
		date = readFixup(binhkx, section);
	}

	return data;
}

glm::quat HavokFile::read40BitQuaternion(Common::ReadStream &binhkx) {
	/*
	 * 40 bit Quaternion structure
	 * - 12 bit x signed integer
	 * - 12 bit y signed integer
	 * - 12 bit z signed integer
	 * - 2 bit shift
	 * - 1 bit invert sign
	 * - 1 bit unused?
	 */
	constexpr float fractal = 0.000345436f;

	glm::quat result(0.0f, 0.0f, 0.0f, 0.0f);

	const uint64_t value = binhkx.readUint64LE();
	binhkx.skip(-3);

	glm::i16vec4 tmp;
	tmp.x = value & 0x0000000000000FFFu;
	tmp.y = (value >> 12u) & 0x0000000000000FFFu;
	tmp.z = (value >> 24u) & 0x0000000000000FFFu;
	tmp.w = 0;

	const unsigned int resultShift = (value >> 36u) & 3u;
	const bool invertSign = (value >> 38u) & 1;

	tmp -= 0x0000000000000801;

	glm::vec4 tmp2(
		glm::vec3(tmp) * fractal,
		0.0f
	);
	tmp2.w = std::sqrt(1.0f - glm::dot(tmp2, tmp2));
	if (invertSign)
		tmp2.w = -tmp2.w;

	for (int i = 0; i < 3 - resultShift; ++i) {
		std::swap(tmp2[3 - i], tmp2[2 - i]);
	}

	result.x = tmp2.x;
	result.y = tmp2.y;
	result.z = tmp2.z;
	result.w = tmp2.w;

	return result;
}

HavokFile::hkArray HavokFile::readHkArray(Common::ReadStream &binhkx, uint32_t section) {
	HavokFile::hkArray array;

	array.offset = readFixup(binhkx, section);
	array.count = binhkx.readUint32LE();
	array.capacityAndFlags = binhkx.readUint32LE();

	return array;
}

void HavokFile::readHkRootLevelContainer(Common::ReadStream &binhkx) {
	/*uint32_t dataOffset = readFixup(binhkx);
	uint32_t numElements = binhkx.readUint32LE();

	binhkx.seek(dataOffset);
	for (int i = 0; i < numElements; ++i) {
		uint32_t nameOffset = readFixup(binhkx);
		uint32_t classNameOffset = readFixup(binhkx);

		uint32_t address = readFixup(binhkx);

		size_t lastPos = binhkx.pos();

		binhkx.seek(nameOffset);
		std::string name = binhkx.readNullTerminatedString();
		binhkx.seek(classNameOffset);
		std::string className = binhkx.readNullTerminatedString();

		binhkx.seek(lastPos);
	}*/

	//fmt::print("RootLevelContainer num_elements={}\n", numElements);
}

void HavokFile::readHkxScene(Common::ReadStream &binhkx) {
	binhkx.skip(8);

	float sceneLength = binhkx.readIEEEFloatLE();

	binhkx.skip(0x44);

	std::vector<glm::vec4> appliedTransform(3);
	for (auto &transform : appliedTransform) {
		transform.x = binhkx.readIEEEFloatLE();
		transform.y = binhkx.readIEEEFloatLE();
		transform.z = binhkx.readIEEEFloatLE();
		transform.w = binhkx.readIEEEFloatLE();
	}

	std::string modeller = binhkx.readNullTerminatedString(4);
	std::string asset = binhkx.readNullTerminatedString(4);
}

std::any HavokFile::readHkaSkeleton(Common::ReadStream &binhkx, uint32_t section) {
	hkaSkeleton s;

	switch (_version) {
		case kHavok2010_2_0_r1: {
			binhkx.skip(8);
			uint32_t nameOffset = readFixup(binhkx, section);

			hkArray parentBoneIndicesArray = readHkArray(binhkx, section);
			hkArray boneArray = readHkArray(binhkx, section);
			hkArray transformArray = readHkArray(binhkx, section);
			hkArray referenceFloatArray = readHkArray(binhkx, section);
			hkArray floatSlotArray = readHkArray(binhkx, section);
			hkArray localFrameArray = readHkArray(binhkx, section);
			hkArray partitionArray = readHkArray(binhkx, section);

			binhkx.seek(nameOffset);
			s.name = binhkx.readNullTerminatedString();

			binhkx.seek(parentBoneIndicesArray.offset);
			std::vector<int16_t> parentIndices(parentBoneIndicesArray.count);
			for (auto &parentIndex : parentIndices) {
				parentIndex = binhkx.readUint16LE();
			}

			binhkx.seek(boneArray.offset);
			for (int i = 0; i < boneArray.count; ++i) {
				uint32_t boneNameOffset = readFixup(binhkx, section);
				bool translationLocked = binhkx.readUint32LE() == 1;

				size_t lastPos = binhkx.pos();
				binhkx.seek(boneNameOffset);
				std::string boneName = binhkx.readNullTerminatedString();
				binhkx.seek(lastPos);

				hkaSkeleton::Bone bone;
				bone.name = boneName;
				bone.translationLocked = translationLocked;
				bone.parentIndex = parentIndices[i];
				s.bones.emplace_back(bone);
			}

			if (transformArray.offset == 0xFFFFFFFF)
				return s;
			binhkx.seek(transformArray.offset);
			for (int i = 0; i < transformArray.count; ++i) {
				glm::vec3 position;
				position.x = binhkx.readIEEEFloatLE();
				position.y = binhkx.readIEEEFloatLE();
				position.z = binhkx.readIEEEFloatLE();
				binhkx.skip(4);

				glm::quat rotation;
				rotation.w = binhkx.readIEEEFloatLE();
				rotation.x = binhkx.readIEEEFloatLE();
				rotation.y = binhkx.readIEEEFloatLE();
				rotation.z = binhkx.readIEEEFloatLE();

				glm::vec3 scale;
				scale.x = binhkx.readIEEEFloatLE();
				scale.y = binhkx.readIEEEFloatLE();
				scale.z = binhkx.readIEEEFloatLE();
				binhkx.skip(4);

				hkaSkeleton::Bone &bone = s.bones[i];
				bone.position = position;
				bone.scale = scale;
				bone.rotation = rotation;
			}
			break;
		}
		case kHavok550_r1: {
			const uint32_t nameOffset = readFixup(binhkx, section);
			const uint32_t parentIndicesOffset = readFixup(binhkx, section);
			const uint32_t numParentIndices = binhkx.readUint32LE();
			const uint32_t bonesOffset = readFixup(binhkx, section);
			const uint32_t numBones = binhkx.readUint32LE();
			const uint32_t transformOffset = readFixup(binhkx, section);
			const uint32_t numTransforms = binhkx.readUint32LE();

			binhkx.seek(nameOffset);
			s.name = binhkx.readNullTerminatedString();

			binhkx.seek(parentIndicesOffset);
			std::vector<int16_t> parentIndices(numParentIndices);
			for (auto &parentIndex : parentIndices) {
				parentIndex = binhkx.readSint16LE();
			}

			const auto boneOffsets = readFixupArray(binhkx, bonesOffset, numBones, section);
			binhkx.seek(bonesOffset);
			//s.bones.resize(numBones);
			for (const auto &boneOffset : boneOffsets) {
				binhkx.seek(boneOffset);

				auto &bone = s.bones.emplace_back();
				const uint32_t boneNameOffset = readFixup(binhkx, section);
				bone.translationLocked = binhkx.readUint32LE() == 1;

				binhkx.seek(boneNameOffset);
				bone.name = binhkx.readNullTerminatedString();
			}

			for (int i = 0; i < parentIndices.size(); ++i) {
				s.bones[i].parentIndex = parentIndices[i];
			}

			binhkx.seek(transformOffset);
			for (int i = 0; i < numTransforms; ++i) {
				auto &bone = s.bones[i];

				bone.position.x = binhkx.readIEEEFloatLE();
				bone.position.y = binhkx.readIEEEFloatLE();
				bone.position.z = binhkx.readIEEEFloatLE();
				binhkx.skip(4);

				bone.rotation.w = binhkx.readIEEEFloatLE();
				bone.rotation.x = binhkx.readIEEEFloatLE();
				bone.rotation.y = binhkx.readIEEEFloatLE();
				bone.rotation.z = binhkx.readIEEEFloatLE();

				bone.scale.x = binhkx.readIEEEFloatLE();
				bone.scale.y = binhkx.readIEEEFloatLE();
				bone.scale.z = binhkx.readIEEEFloatLE();
				binhkx.skip(4);
			}
			break;
		}
	}

	/*binhkx.seek(referenceFloatArray.offset);
	for (int i = 0; i < referenceFloatArray.count; ++i) {
		s.referenceFloats.emplace_back(binhkx.readIEEEFloatLE());
	}*/

	/*std::vector<uint32_t> floatSlotNameAddresses = readFixupArray(binhkx, floatSlotArray, section);
	for (const auto &nameAddress : floatSlotNameAddresses) {
		binhkx.seek(nameAddress);
		std::string name = binhkx.readNullTerminatedString();
		std::cout << name << std::endl;
	}*/

	return s;
}

HavokFile::hkaAnimation HavokFile::readHkaSplineCompressedAnimation(Common::ReadStream &binhkx, uint32_t section) {
	hkaAnimation animation;

	uint32_t unknown0 = binhkx.readUint32LE();
	uint32_t unknown1 = binhkx.readUint32LE();

	uint32_t type = binhkx.readUint32LE();

	animation.duration = binhkx.readIEEEFloatLE();

	uint32_t numTranformTracks = binhkx.readUint32LE();
	uint32_t numFloatTracks = binhkx.readUint32LE();

	uint32_t extractedMotion = readFixup(binhkx, section);

	hkArray annotationTracks = readHkArray(binhkx, section);
	
	uint32_t numFrames = binhkx.readUint32LE();
	uint32_t numBlocks = binhkx.readUint32LE();
	uint32_t maxFramesPerBlock= binhkx.readUint32LE();
	uint32_t maskAndQuantizationSize = binhkx.readUint32LE();
	float blockDuration = binhkx.readIEEEFloatLE();
	float blockInverseDuration = binhkx.readIEEEFloatLE();
	float frameDuration = binhkx.readIEEEFloatLE();

	hkArray blockOffsetsArray = readHkArray(binhkx, section);
	hkArray floatBlockOffsets = readHkArray(binhkx, section);
	hkArray transformOffsets = readHkArray(binhkx, section);
	hkArray floatOffsets = readHkArray(binhkx, section);
	hkArray data = readHkArray(binhkx, section);

	binhkx.seek(data.offset);
	std::unique_ptr<Common::ReadStream> dataStream(binhkx.readStream(data.count));

	std::vector<uint32_t> blockOffsets = readUint32Array(binhkx, blockOffsetsArray);
	for (const auto &offset : blockOffsets) {
		dataStream->seek(offset);

		// Read Transform masks
		std::vector<TransformMask> masks(numTranformTracks);
		for (auto &mask : masks) {
			mask.quantizationTypes = dataStream->readByte();
			mask.positionTypes     = dataStream->readByte();
			mask.rotationTypes     = dataStream->readByte();
			mask.scaleTypes        = dataStream->readByte();
		}

		size_t begin = dataStream->pos();
		unsigned int frames = 0;

		for (const auto &mask : masks) {
			const bool transformSplineX = (mask.positionTypes & 0x10) != 0;
			const bool transformSplineY = (mask.positionTypes & 0x20) != 0;
			const bool transformSplineZ = (mask.positionTypes & 0x40) != 0;
			const bool transformStaticX = (mask.positionTypes & 0x01) != 0;
			const bool transformStaticY = (mask.positionTypes & 0x02) != 0;
			const bool transformStaticZ = (mask.positionTypes & 0x04) != 0;
			const bool rotationTypeSpline = (mask.rotationTypes &0xf0) != 0;
			const bool rotationTypeStatic = (mask.rotationTypes &0x0f) != 0;
			const bool scaleSplineX = (mask.scaleTypes & 0x10) != 0;
			const bool scaleSplineY = (mask.scaleTypes & 0x20) != 0;
			const bool scaleSplineZ = (mask.scaleTypes & 0x40) != 0;
			const bool scaleStaticX = (mask.scaleTypes & 0x01) != 0;
			const bool scaleStaticY = (mask.scaleTypes & 0x02) != 0;
			const bool scaleStaticZ = (mask.scaleTypes & 0x04) != 0;

			const bool transformSpline = transformSplineX || transformSplineY || transformSplineZ;
			const bool transformStatic = transformStaticX || transformStaticY || transformStaticZ;
			const bool scaleSpline = scaleSplineX || scaleSplineY || scaleSplineZ;

			hkaAnimation::Track track{};

			auto positionQuantizationType = QuantizationType(mask.quantizationTypes & 0x03);
			auto rotationQuantizationType = QuantizationType(((mask.quantizationTypes >> 2u) & 0x0f) + 2);
			auto scaleQuantizationType    = QuantizationType((mask.quantizationTypes >> 6u) & 0x03);

			if (transformStatic || transformSpline)
				track.positions = std::vector<glm::vec3>();

			if (transformSpline) {
				uint16_t numItems = dataStream->readUint16LE();
				frames += numItems;
				uint8_t degree = dataStream->readByte();

				dataStream->skip(numItems + degree + 2);
				if ((dataStream->pos() - begin) % 4 != 0)
					dataStream->skip(4 - (dataStream->pos() - begin) % 4);

				float minx = 0, maxx = 0, miny = 0, maxy = 0, minz = 0, maxz = 0;
				float staticx = 0, staticy = 0, staticz = 0;
				if (transformSplineX) {
					minx = dataStream->readIEEEFloatLE();
					maxx = dataStream->readIEEEFloatLE();
				} else if (transformStaticX) {
					staticx = dataStream->readIEEEFloatLE();
				}

				if (transformSplineY) {
					miny = dataStream->readIEEEFloatLE();
					maxy = dataStream->readIEEEFloatLE();
				} else if (transformStaticY) {
					staticy = dataStream->readIEEEFloatLE();
				}

				if (transformSplineZ) {
					minz = dataStream->readIEEEFloatLE();
					maxz = dataStream->readIEEEFloatLE();
				} else if (transformStaticZ) {
					staticz = dataStream->readIEEEFloatLE();
				}

				for (int i = 0; i <= numItems; ++i) {
					glm::vec3 position(0);
					switch (positionQuantizationType) {
						case k8Bit:
							if (transformSplineX)
								position.x = static_cast<float>(dataStream->readByte()) * (1.0f / 255.0f);
							if (transformSplineY)
								position.y = static_cast<float>(dataStream->readByte()) * (1.0f / 255.0f);
							if (transformSplineZ)
								position.z = static_cast<float>(dataStream->readByte()) * (1.0f / 255.0f);
							break;

						case k16Bit:
							if (transformSplineX)
								position.x = static_cast<float>(dataStream->readUint16LE()) * (1.0f / 65535.0f);
							if (transformSplineY)
								position.y = static_cast<float>(dataStream->readUint16LE()) * (1.0f / 65535.0f);
							if (transformSplineZ)
								position.z = static_cast<float>(dataStream->readUint16LE()) * (1.0f / 65535.0f);
							break;

						default:
							throw std::runtime_error("Invalid Quantization");
					}

					position.x = minx + (maxx - minx) * position.x;
					position.y = miny + (maxy - miny) * position.y;
					position.z = minz + (maxz - minz) * position.z;

					if (!transformSplineX)
						position.x = staticx;
					if (!transformSplineY)
						position.y = staticy;
					if (!transformSplineZ)
						position.z = staticz;

					track.positions->emplace_back(position);
				}

				if ((dataStream->pos() - begin) % 4 != 0)
					dataStream->skip(4 - (dataStream->pos() - begin) % 4);
			} else {
				glm::vec3 position(0);
				if (transformStaticX)
					position.x = dataStream->readIEEEFloatLE();
				if (transformStaticY)
					position.y = dataStream->readIEEEFloatLE();
				if (transformStaticZ)
					position.z = dataStream->readIEEEFloatLE();

				track.positions->emplace_back(position);
			}

			if (rotationTypeSpline) {
				//uint64_t savePtr = dataStream->readUint64LE();
				uint16_t numItems = dataStream->readUint16LE();
				uint8_t degree = dataStream->readByte();

				dataStream->skip(numItems + degree + 2);

				for (int i = 0; i <= numItems; ++i) {
					glm::quat rotation;

					switch (rotationQuantizationType) {
						case k40Bit:
							rotation = read40BitQuaternion(*dataStream);
							break;
						default:
							throw std::runtime_error("Invalid quantization type");
					}

					track.rotations.emplace_back(rotation);
				}
			} else if (rotationTypeStatic) {
				glm::quat rotation;
				switch (rotationQuantizationType) {
					case k40Bit:
						rotation = read40BitQuaternion(*dataStream);
						break;
					default:
						throw std::runtime_error("Invalid quantization type");
				}
				track.rotations.emplace_back(rotation);
			}

			if ((dataStream->pos() - begin) % 4 != 0)
				dataStream->skip(4 - (dataStream->pos() - begin) % 4);

			if (scaleSpline) {
				assert(false);
			} else {
				if (scaleStaticX)
					dataStream->skip(4);
				if (scaleStaticY)
					dataStream->skip(4);
				if (scaleStaticZ)
					dataStream->skip(4);

			}

			animation.tracks.emplace_back(track);
		}
	}

	binhkx.seek(annotationTracks.offset);
	for (int i = 0; i < annotationTracks.count; ++i) {
		uint32_t nameOffset = readFixup(binhkx, section);
        uint32_t unknown1 = binhkx.readUint32LE();
        //uint32_t offset1 = readFixup(binhkx, section);
        uint32_t unknown2 = binhkx.readUint32LE();
		//uint32_t offset2 = readFixup(binhkx, section);
		float unknown8 = binhkx.readIEEEFloatLE();

		size_t lastPos = binhkx.pos();
		binhkx.seek(nameOffset);
		std::string boneName = binhkx.readNullTerminatedString();
		binhkx.seek(lastPos);

		animation.boneToTrack[boneName] = i;
	}

	return animation;
}

void HavokFile::readHkaAnimationBinding(Common::ReadStream &binhkx, uint32_t section) {
	hkaAnimationBinding binding;

	binhkx.skip(8);

	uint32_t nameOffset = readFixup(binhkx,  section);
	uint32_t animation = readFixup(binhkx, section);

	hkArray transformTrackToBoneIndexArray = readHkArray(binhkx, section);
	hkArray floatTrackToFloatSlotIndexArray = readHkArray(binhkx, section);
	hkArray partitionIndices = readHkArray(binhkx, section);

	binhkx.seek(nameOffset);
	binding.skeletonName = binhkx.readNullTerminatedString();
	binding.animation = animation;

	binding.transformTrackToBoneIndices.resize(transformTrackToBoneIndexArray.count);
	binhkx.seek(transformTrackToBoneIndexArray.offset);
	for (auto &item : binding.transformTrackToBoneIndices) {
		item = binhkx.readSint16LE();
	}
}

void HavokFile::readHkaAnimationContainer(Common::ReadStream &binhkx, uint32_t section) {
	switch (_version) {
		case kHavok2010_2_0_r1: {
			binhkx.skip(8);

			hkArray skeletons = readHkArray(binhkx, section);
			hkArray animations = readHkArray(binhkx, section);
			hkArray bindings = readHkArray(binhkx, section);
			hkArray boneAttachments = readHkArray(binhkx, section);

			_animationContainer.skeletons = readFixupArray(binhkx, skeletons, section);
			_animationContainer.animations = readFixupArray(binhkx, animations, section);
			_animationContainer.bindings = readFixupArray(binhkx, bindings, section);

			break;
		}

		case kHavok550_r1: {
			const uint32_t skeletons = readFixup(binhkx, section);
			const uint32_t numSkeletons = binhkx.readUint32LE();
			const uint32_t animations = readFixup(binhkx, section);
			const uint32_t numAnimations = binhkx.readUint32LE();

			_animationContainer.skeletons = readFixupArray(binhkx, skeletons, numSkeletons, section);
		}
	}
}

HavokFile::RmdPhysicsSystem HavokFile::readRmdPhysicsSystem(Common::ReadStream &binhkx, uint32_t section) {
	RmdPhysicsSystem rmdPhysicsSystem{};

	binhkx.skip(8);

	hkArray rigidBodiesArray = readHkArray(binhkx, section);
	hkArray array2 = readHkArray(binhkx, section);
	hkArray array3 = readHkArray(binhkx, section);
	hkArray array4 = readHkArray(binhkx, section);

	binhkx.skip(12);

	hkArray nameArray = readHkArray(binhkx, section);
	hkArray array6 = readHkArray(binhkx, section);
	hkArray array7 = readHkArray(binhkx, section);

	rmdPhysicsSystem.rigidBodies = readFixupArray(binhkx, rigidBodiesArray, section);
	std::vector<uint32_t> nameOffsets = readFixupArray(binhkx, nameArray, section);
	std::vector<std::string> names;
	for (const auto &offset : nameOffsets) {
		binhkx.seek(offset);
		names.emplace_back(binhkx.readNullTerminatedString());
	}

	return rmdPhysicsSystem;
}

HavokFile::hkpRigidBody HavokFile::readHkpRigidBody(Common::ReadStream &binhkx, uint32_t section) {
	hkpRigidBody rigidBody{};

	binhkx.skip(16);
	rigidBody.shape = readFixup(binhkx, section);

	return rigidBody;
}

HavokFile::hkpShape HavokFile::readHkpBoxShape(Common::ReadStream &binhkx) {
	hkpShape shape{};
	hkpBoxShape boxShape{};

	shape.type = kBox;

	binhkx.skip(8); // hkReferencedObject
	shape.userData = binhkx.readUint64LE(); // hkpShape
	shape.radius = binhkx.readIEEEFloatLE(); // hkpConvexShape
	binhkx.skip(12);

	glm::vec4 halfExtents;
	halfExtents.x = binhkx.readIEEEFloatLE();
	halfExtents.y = binhkx.readIEEEFloatLE();
	halfExtents.z = binhkx.readIEEEFloatLE();
	halfExtents.w = binhkx.readIEEEFloatLE();
	boxShape.halfExtents = halfExtents;
	shape.shape = boxShape;

	return shape;
}

void HavokFile::setHeader(std::string headerVersion) {
	if (headerVersion == "Havok-5.5.0-r1")
		_version = kHavok550_r1;
	else if (headerVersion == "hk_2010.2.0-r1")
		_version = kHavok2010_2_0_r1;
	else
		throw std::runtime_error(fmt::format("Unsupported havok version {}", headerVersion));

}

uint32_t HavokFile::readFixup(Common::ReadStream &binhkx, uint32_t section) {
	if (_fixups.find(binhkx.pos() - _sectionOffsets[section]) == _fixups.end()) {
		binhkx.skip(4);
		return 0xffffffff;
	}

	Fixup fixup = _fixups[binhkx.pos() - _sectionOffsets[section]];
	if (fixup.targetAddress == 0)
		throw std::runtime_error("Invalid fixup");

	binhkx.skip(4);

	if (fixup.section)
		fixup.targetAddress += _sectionOffsets[*fixup.section];

	return fixup.targetAddress;
}

} // End of namespace AWE
