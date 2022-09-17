#ifndef __DATASERIALIZER__H__
#define __DATASERIALIZER__H__

#include <string>
#include <RVCore/dataFileManager.h>
#include <RVCore/math.h>

namespace rv
{
	// TODO: Create Versioned Data Serializer
	class DataSerializer
	{
		using string = std::string;

	  public:
		DataSerializer() = delete;
		~DataSerializer() = default;
		DataSerializer(DataFileManager* const fileManager, string dataPath)
		    : fileManager(fileManager), dataPathEntry(crc32(dataPath))
		{
		}

		inline bool Serialize()
		{
			DataWriter dataWriter;
			bool foundEntry = fileManager->GetDataWriter(dataPathEntry, dataWriter);
			Serialize(dataWriter);
			dataWriter.Close();
			return foundEntry;
		}
		inline bool Deserialize()
		{
			DataReader dataReader;
			if (fileManager->TryGetDataReader(dataPathEntry, dataReader))
			{
				Deserialize(dataReader);
				return true;
			}
			return false;
		}

	  protected:
		virtual void Serialize(DataWriter& dataWriter) = 0;
		virtual void Deserialize(DataReader& dataReader) = 0;

	  private:
		DataFileManager* fileManager;
		uint32_t dataPathEntry;
	};

} // namespace rv

#endif //!__DATASERIALIZER__H__