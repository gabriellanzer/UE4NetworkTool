#ifndef __PROJECTMANAGER__H__
#define __PROJECTMANAGER__H__

#include <fstream>
#include <iostream>

#include <string>
#include <vector>
#include <fmt/core.h>

#include <RVCore/dataFileManager.h>
#include <RVCore/resourceManager.h>
#include <RVCore/resourceLoader.h>
#include <RVCore/loaderManager.h>
#include <RVCore/utils.h>

namespace rv
{
	class ProjectManager
	{
		using string = std::string;
		template <class T>
		using vector = std::vector<T>;

	  public:
		ProjectManager();
		ProjectManager(string fileExtension);
		~ProjectManager();

		void SetProjectDir(string newProjDir);
		void SetProjectName(string newProjName);
		string* GetProjectDirPtr();
		string* GetProjectNamePtr();
		string GetProjectFilePath();

		string Encode();
		void Decode(vector<string> encodedTokens, uint32_t& tokenOffset);

		void NewProject(string folderPath, string projectName);
		void LoadProject(string filePath);
		void SaveProject();

		inline bool IsInitialized() { return initialized; }
		string GetFileExtension() { return fileExtension; }

		LoaderManager* const loaderManager;
		ResourcesManager* const rscManager;
		ResourceLoader* const rscLoader;
		DataFileManager* const dataFileManager;

	  private:
		bool initialized;
		string fileExtension;
		string projectName;
		string projectDirectory;
	};

	inline ProjectManager::ProjectManager()
	    : fileExtension(".ravineproj"), projectName("Unnamed Project"), projectDirectory(""),
	      loaderManager(new LoaderManager()), rscManager(new ResourcesManager()),
	      rscLoader(new ResourceLoader(loaderManager, rscManager)), dataFileManager(new rv::DataFileManager()),
	      initialized(false)
	{
		rscManager->SetProjectDir(&projectDirectory);
		rscLoader->SetProjectDir(&projectDirectory);
	}

	inline ProjectManager::ProjectManager(string fileExtension)
	    : fileExtension(fileExtension), projectName("Unnamed Project"), loaderManager(new rv::LoaderManager()),
	      rscManager(new rv::ResourcesManager()), rscLoader(new rv::ResourceLoader(loaderManager, rscManager)),
	      dataFileManager(new rv::DataFileManager()), initialized(false)
	{
		rscManager->SetProjectDir(&projectDirectory);
		rscLoader->SetProjectDir(&projectDirectory);
	}

	inline ProjectManager::~ProjectManager()
	{
		delete loaderManager;
		delete rscManager;
		delete rscLoader;
		delete dataFileManager;
	}

	inline void ProjectManager::SetProjectDir(std::string newProjDir) { projectDirectory = newProjDir; }

	inline void ProjectManager::SetProjectName(std::string newProjName) { projectName = newProjName; }

	inline std::string ProjectManager::GetProjectFilePath()
	{
		return projectDirectory + projectName + fileExtension;
	}

	inline std::string* ProjectManager::GetProjectDirPtr() { return &projectDirectory; }
	inline std::string* ProjectManager::GetProjectNamePtr() { return &projectName; }

	inline std::string ProjectManager::Encode() { return fmt::format("{}\n", projectName.c_str()).c_str(); }

	inline void ProjectManager::Decode(vector<string> encodedTokens, uint32_t& tokenOffset)
	{
		SetProjectName(encodedTokens[tokenOffset]);
		tokenOffset++;
	}

	inline void ProjectManager::NewProject(string folderPath, string projectName)
	{
		SetProjectDir(folderPath);
		SetProjectName(projectName);

		// Bind data file
		dataFileManager->BindDataFile(folderPath);

		// Set flags
		initialized = true;
	}

	inline void ProjectManager::LoadProject(string filePath)
	{
		std::fstream fStream;
		fStream.open(filePath.c_str(), std::ios::in | std::ios::binary);
		if (!fStream.is_open())
		{
			return;
		}

		string fileName;
		string fileExt;
		string directory = splitFilename(filePath, fileName, fileExt);
		SetProjectDir(directory);
		SetProjectName(fileName);

		std::streamoff fileSize;
		fStream.seekg(0, std::ios::end);
		fileSize = fStream.tellg();
		fStream.seekg(0, std::ios::beg);

		// Load file data into string
		std::string dataStr;
		dataStr.resize(fileSize);
		fStream.read(dataStr.data(), fileSize);
		fStream.close();

		// Split values by line
		vector<string> dataTokens = splitStr(dataStr, "\n");

		// Invoke Decodes
		uint32_t tokenOffset = 0;
		this->Decode(dataTokens, tokenOffset);
		rscManager->Decode(dataTokens, tokenOffset);
		rscLoader->LoadAllResources();

		// Bind and load data file
		dataFileManager->Reset();
		dataFileManager->BindDataFile(directory);
		dataFileManager->LoadIndexTable();

		// Set flags
		initialized = true;
	}

	inline void ProjectManager::SaveProject()
	{
		std::fstream fStream;
		string projectPath = projectDirectory + projectName + fileExtension;
		fStream.open(projectPath.c_str(), std::ios::out | std::ios::binary);
		if (!fStream.is_open())
		{
			return;
		}
		fStream << this->Encode().c_str();
		fStream << rscManager->Encode().c_str();
		fStream.close();
		dataFileManager->SaveIndexTable();
	}
} // namespace rv

#endif //!__PROJECTMANAGER__H__