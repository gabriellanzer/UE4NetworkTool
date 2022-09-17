#ifndef __DATAFILEMANAGER__H__
#define __DATAFILEMANAGER__H__

#include <map>

#include <fstream>
#include <ios>
#include <cstdint>

#include <RVCore/resource.h>
#include <RVCore/utils.h>

namespace rv
{
	class DataReader
	{
		using fstream = std::fstream;
		using ios = std::ios;

		// TODO: Handle Reading Out of existing Data Block Size

	  public:
		DataReader() = default;
		DataReader(fstream* dataStream, size_t fileOffset, size_t dataFileSize)
		    : dataStream(dataStream), readIniPos(fileOffset), dataFileSize(dataFileSize), readOffset(0)
		{
			dataStream->seekg(readIniPos, ios::beg);
			// Read Data Block Size from the beggining of the file entry
			char* dataBlockSizeBuf = reinterpret_cast<char*>(&dataBlockSize);
			dataStream->read(dataBlockSizeBuf, sizeof(size_t));
		}

		template <typename TData>
		void Read(TData&& dataRef, size_t readOffset)
		{
			dataStream->seekg(readOffset, dataStream->cur);
			Read(dataRef);
		}

		template <typename TData>
		void Read(TData&& dataRef)
		{
			size_t dataSize = sizeof(TData);
			_ASSERT(dataSize <= dataFileSize - readOffset);
			// throw "Reading out of bounds of Data File!";

			char* dataBuf = new char[8];
			dataStream->read(dataBuf, dataSize);
			using TSourceData = typename TRefSource<TData>::Type;
			dataRef = *reinterpret_cast<TSourceData*>(dataBuf);
		}

	  private:
		size_t dataBlockSize;
		size_t dataFileSize;
		size_t readOffset;
		size_t readIniPos;
		fstream* dataStream;
	};

	class DataWriter
	{
		using fstream = std::fstream;
		using ios = std::ios;

		// TODO: Handle Writting Out of existing Data Block Size (when overwritting existing data)

	  public:
		DataWriter() = default;

		DataWriter(fstream* dataStream, size_t fileOffset, size_t dataFileSize)
		    : dataStream(dataStream), writeIniPos(fileOffset), dataFileSize(dataFileSize), writeOffset(0),
		      dataBlockSize(0)
		{
			dataStream->seekp(writeIniPos, ios::beg);
			// Write (zeroed) Data Block Size at the beggining of the file entry
			char* dataBlockSizeBuf = reinterpret_cast<char*>(&dataBlockSize);
			dataStream->write(dataBlockSizeBuf, sizeof(size_t));
		}

		void Close()
		{
			dataStream->seekp(writeIniPos, ios::beg);
			// Write Data Block Size at the beggining of the file entry
			char* dataBlockSizeBuf = reinterpret_cast<char*>(&dataBlockSize);
			dataStream->write(dataBlockSizeBuf, sizeof(size_t));
			dataStream->flush();
		}

		template <typename TData>
		void Write(const TData& data, size_t writeOffset)
		{
			dataStream->seekp(writeOffset, dataStream->cur);
			Write(data);
		}

		template <typename TData>
		void Write(const TData& data)
		{
			size_t dataSize = sizeof(TData);
			const char* dataBuf = reinterpret_cast<const char*>(&data);
			dataStream->write(dataBuf, dataSize);
			dataBlockSize += dataSize;
		}

	  private:
		size_t dataBlockSize;
		size_t dataFileSize;
		size_t writeOffset;
		size_t writeIniPos;
		fstream* dataStream;
	};

	class DataFileManager
	{
		template <typename TKey, typename TValue>
		using map = std::map<TKey, TValue>;
		using string = std::string;
		using fStream = std::fstream;
		using ios = std::ios;

	  public:
		DataFileManager() = default;
		~DataFileManager() { Reset(); }

		inline void BindDataFile(string projectDir)
		{
			string dataFilePath = projectDir + "/project.dat";
			string indexFilePath = projectDir + "/project.idx";
			if (!fileExists(dataFilePath.c_str()) || !fileExists(indexFilePath.c_str()))
			{
				// Create files if they don't exist
				dataFile.open(dataFilePath.c_str(), ios::out | ios::binary);
				indexFile.open(indexFilePath.c_str(), ios::out | ios::binary);
				dataFile.close();
				indexFile.close();
			}

			dataFile.open(dataFilePath.c_str(), ios::in | ios::out | ios::binary);
			indexFile.open(indexFilePath.c_str(), ios::in | ios::out | ios::binary);

			_ASSERT(dataFile.is_open());
			_ASSERT(indexFile.is_open());

			dataFile.seekg(0, ios::end);
			fileSize = dataFile.tellg();
			dataFile.seekg(0, ios::beg);
		}

		inline void Reset()
		{
			if (dataFile.is_open())
			{
				dataFile.close();
			}
			if (indexFile.is_open())
			{
				indexFile.close();
			}
			dataIndexTable.clear();
		}

		inline bool TryGetDataReader(uint32_t dataEntryId, DataReader& dataReader)
		{
			auto entry = dataIndexTable.find(dataEntryId);
			if (entry == dataIndexTable.end())
				return false;

			_ASSERT(entry->second < fileSize);
			// throw "Data Entry out of Data file bounds!";

			dataReader = DataReader(&dataFile, entry->second, fileSize);
			return true;
		}

		inline bool GetDataWriter(uint32_t dataEntryId, DataWriter& dataWriter)
		{
			auto entry = dataIndexTable.lower_bound(dataEntryId);
			// Should the entry on the index table exist
			if (entry != dataIndexTable.end() && entry->first == dataEntryId)
			{
				_ASSERT(entry->second <= fileSize);
				// throw "Data Entry out of Data file bounds!";

				dataWriter = DataWriter(&dataFile, entry->second, fileSize);
				return true;
			}

			dataIndexTable.emplace_hint(entry, dataEntryId, fileSize);
			dataWriter = DataWriter(&dataFile, fileSize, fileSize);
			return false;
		}

		inline void LoadIndexTable()
		{
			indexFile.seekg(0, ios::beg);
			size_t entryCount;
			indexFile.read(reinterpret_cast<char*>(&entryCount), sizeof(size_t));
			for (size_t i = 0; i < entryCount; i++)
			{
				uint32_t dataEntryId;
				indexFile.read(reinterpret_cast<char*>(&dataEntryId), sizeof(uint32_t));
				size_t dataPosition;
				indexFile.read(reinterpret_cast<char*>(&dataPosition), sizeof(size_t));
				dataIndexTable.emplace(dataEntryId, dataPosition);
			}
		}

		inline void SaveIndexTable()
		{
			indexFile.seekp(0, ios::beg);
			const size_t indexSize = dataIndexTable.size();
			const char* dataPtr = reinterpret_cast<const char*>(&indexSize);
			indexFile.write(dataPtr, sizeof(size_t));
			for (const auto& entry : dataIndexTable)
			{
				dataPtr = reinterpret_cast<const char*>(&entry.first);
				indexFile.write(dataPtr, sizeof(uint32_t));
				dataPtr = reinterpret_cast<const char*>(&entry.second);
				indexFile.write(dataPtr, sizeof(size_t));
			}
			indexFile.flush();
		}

	  private:
		fStream dataFile;
		fStream indexFile;
		size_t fileSize;
		map<uint32_t, size_t> dataIndexTable;
	};

} // namespace rv

#endif //!__DATAFILEMANAGER__H__