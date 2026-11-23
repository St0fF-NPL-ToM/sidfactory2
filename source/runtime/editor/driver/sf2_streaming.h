/**
 *  SF2 pack/unpack byte streaming
 */

#include "runtime/editor/interfaces/imemory_streaming.h"

namespace Editor
{

	struct SF2_OrderStreaming : public IOrderStreaming
	{
		DataSourceOrderList::PackResult PackOrderList(const DataSourceOrderList& inData, unsigned char* m_InternalBuffer) const override;
		DataSourceOrderList::PackedDataEventPosition GetOrderListIndex(int inIndex, const unsigned char* inData, const int inDataSize) const override;
		std::pair<int, int> UnpackOrderList(DataSourceOrderList& outData, unsigned char* m_Data) const override;

		// Orderlist-Stream
		// bool IsOrderListEntrySequenceIndex(unsigned char inValue, unsigned char& outIndex) const override;
		// bool IsOrderListTerminator(unsigned char inValue, bool& outHasLoopIndex) const override;
	};


	struct SF2_SequenceStreaming : public ISequenceStreaming
	{
		DataSourceSequence::PackResult PackSequence(const DataSourceSequence& inData, unsigned char* m_InternalBuffer) const override;
		bool MayPushSequenceData(const DataSourceSequence& inData) const override;
		DataSourceSequence::PackedDataEventPosition GetSequenceIndex(int inEventPosition, const unsigned char* m_Data, const int m_PackedSize) const override;
		DataSourceSequence::UnpackResult UnpackSequence(DataSourceSequence& outData, unsigned char* m_Data) const override;

		// Sequence-Stream
		// bool IsSequenceTerminator(unsigned char inValue) const override;
		// bool DecodeSequenceEventByte(unsigned char* inData, EventKind& outKind, unsigned char& outIndex) const override;
		// bool IsEmptySequence(const unsigned char* inData) const override;
	};
}
