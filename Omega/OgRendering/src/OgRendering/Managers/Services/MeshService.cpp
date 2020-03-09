#include <OgRendering/Managers/Services/MeshService.h>
#include <OgRendering/Managers/Loaders/LoaderManager.h>
#include <sstream>

OgEngine::Services::MeshService::MeshService()
= default;

OgEngine::Services::MeshService::~MeshService()
{
	WaitForAll();
	m_workerToMesh.clear();
	m_meshes.clear();
}

void OgEngine::Services::MeshService::Add(std::string_view p_filePath)
{
	const std::string_view fileName{ p_filePath.data() + (p_filePath.find_last_of('/') + 1) };

	if (m_meshes.find(fileName.data()) != m_meshes.end())
	{
		std::cout << "Warning: The file '" << fileName << "' already exist in memory, loading is discarded.\n";
	}

	m_meshes.insert({ fileName.data(), std::make_shared<Mesh>() });

	m_meshes.at(fileName.data())->SetHashID(m_hashValueFromName(p_filePath.data()));
	
	m_workerToMesh.emplace_back(std::make_pair<uint64_t, std::string>(m_pool.WorkersInUse(), fileName.data()));
	
	m_pool.AddTask(&MeshService::MultithreadedLoading, this, p_filePath);
}

void OgEngine::Services::MeshService::MultithreadedLoading(std::string_view p_filePath)
{
	const std::string_view fileName{ p_filePath.data() + (p_filePath.find_last_of('/') + 1) };
	m_meshes.at(fileName.data())->FillData(LoaderManager::Load<Mesh>(p_filePath));
}

inline std::shared_ptr<OgEngine::Mesh> OgEngine::Services::MeshService::Get(std::string_view p_meshName) const
{
	if (m_meshes.find(p_meshName.data()) != m_meshes.end())
		return m_meshes.at(p_meshName.data());

	return nullptr;
}

inline void OgEngine::Services::MeshService::WaitForResource(std::string_view p_meshName)
{
	const auto& pairFound = std::find_if(m_workerToMesh.begin(), m_workerToMesh.end(),
		[p_meshName](const std::pair<uint64_t, std::string>& element)
	{ return element.second == p_meshName.data(); });

	if (pairFound != m_workerToMesh.end())
	{
		m_pool.WaitForWorker(pairFound->first);
		m_workerToMesh.erase(pairFound);
	}
	else
		std::cerr << "Couldn't find " << p_meshName.data() << ", waiting for the Resource skipped.\nThe resource might be already into memory or the file name is misspelled.\n";
}
