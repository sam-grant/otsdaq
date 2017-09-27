#include "otsdaq-core/ConfigurationInterface/DatabaseConfigurationInterface.h"
#include "otsdaq-core/ConfigurationDataFormats/ConfigurationBase.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"
#include "otsdaq-core/Macros/CoutHeaderMacros.h"


#include <iostream>
#include <string>
#include <algorithm>
#include <iterator>

#include "artdaq-database/ConfigurationDB/configurationdbifc.h"
#include "artdaq-database/BasicTypes/basictypes.h"

#include "artdaq-database/StorageProviders/FileSystemDB/provider_filedb.h"
#include "artdaq-database/StorageProviders/FileSystemDB/provider_filedb_index.h"
#include "artdaq-database/ConfigurationDB/configuration_common.h"
#include "artdaq-database/ConfigurationDB/dispatch_common.h"
#include "JSONConfigurationHandler.h.unused"



using namespace ots;

using artdaq::database::basictypes::FhiclData;
using artdaq::database::basictypes::JsonData;

using ots::DatabaseConfigurationInterface;
using config_version_map_t = ots::DatabaseConfigurationInterface::config_version_map_t;

namespace db = artdaq::database::configuration;
using VersionInfoList_t = db::ConfigurationInterface::VersionInfoList_t;

constexpr auto default_dbprovider = "filesystem";
constexpr auto default_entity = "OTSROOT";

//==============================================================================
DatabaseConfigurationInterface::DatabaseConfigurationInterface()
{
#ifdef DEBUG_ENABLE
	//to enable debugging
	if(0)
	{
  artdaq::database::configuration::debug::ExportImport();
  artdaq::database::configuration::debug::ManageAliases();
  artdaq::database::configuration::debug::ManageConfigs();
  artdaq::database::configuration::debug::ManageDocuments();
  artdaq::database::configuration::debug::Metadata();
  
  artdaq::database::configuration::debug::detail::ExportImport();
  artdaq::database::configuration::debug::detail::ManageAliases();
  artdaq::database::configuration::debug::detail::ManageConfigs();
  artdaq::database::configuration::debug::detail::ManageDocuments();
  artdaq::database::configuration::debug::detail::Metadata();

  artdaq::database::configuration::debug::options::OperationBase();
  artdaq::database::configuration::debug::options::BulkOperations();
  artdaq::database::configuration::debug::options::ManageDocuments();
  artdaq::database::configuration::debug::options::ManageConfigs();
  artdaq::database::configuration::debug::options::ManageAliases();
  
  artdaq::database::configuration::debug::MongoDB();
  artdaq::database::configuration::debug::UconDB();
  artdaq::database::configuration::debug::FileSystemDB();
  
  artdaq::database::filesystem::index::debug::enable();
  
  artdaq::database::filesystem::debug::enable();
  artdaq::database::mongo::debug::enable();

  artdaq::database::docrecord::debug::JSONDocumentBuilder();
  artdaq::database::docrecord::debug::JSONDocument();

  //debug::registerUngracefullExitHandlers();
  //  artdaq::database::useFakeTime(true);

	}
#endif

}

//==============================================================================
// read configuration from database
// version = -1 means latest version
void DatabaseConfigurationInterface::fill(ConfigurationBase* configuration, ConfigurationVersion version) const
throw(std::runtime_error)
{
	auto ifc = db::ConfigurationInterface{default_dbprovider};

	auto versionstring = version.toString();

	//configuration->getViewP()->setUniqueStorageIdentifier(storageUID);

	auto result =
			ifc.template loadVersion<decltype(configuration), JsonData>(configuration, versionstring, default_entity);

	if (result.first)
	{
		//make sure version is set.. not clear it was happening in loadVersion
		configuration->getViewP()->setVersion(version);
		return;
	}
	__SS__ << "\n\nDBI Error while filling '" << configuration->getConfigurationName() <<
			"' version '" << versionstring << "' - are you sure this version exists?" <<
			"Here is the error:\n\n" << result.second << std::endl;
	std::cout << __COUT_HDR_FL__ << "\n" << ss.str();
	throw std::runtime_error(ss.str());
}

//==============================================================================
// write configuration to database
void DatabaseConfigurationInterface::saveActiveVersion(const ConfigurationBase*
		configuration, bool overwrite) const
throw(std::runtime_error)
{
	auto ifc = db::ConfigurationInterface{default_dbprovider};

	//configuration->getView().getUniqueStorageIdentifier()

	auto versionstring = configuration->getView().getVersion().toString();
	//std::cout << __COUT_HDR_FL__ << "versionstring: " << versionstring << "\n";

	//auto result =
		//	ifc.template storeVersion<decltype(configuration), JsonData>(configuration, versionstring, default_entity);
	 auto result = overwrite ?
	      ifc.template overwriteVersion<decltype(configuration), JsonData>(configuration, versionstring, default_entity) :
	      ifc.template storeVersion<decltype(configuration), JsonData>(configuration, versionstring, default_entity);

	if (result.first)
		return;

	__SS__ << "DBI Error:" << result.second << std::endl;
	std::cout << __COUT_HDR_FL__ << "\n" << ss.str();
	throw std::runtime_error(ss.str());
}

//==============================================================================
// find the latest configuration version by configuration type
ConfigurationVersion DatabaseConfigurationInterface::findLatestVersion(const ConfigurationBase* configuration) const
noexcept
{
	auto versions = getVersions(configuration);

	std::cout << __COUT_HDR_FL__ << "Config Name: " << configuration->getConfigurationName() << std::endl;
	std::cout << __COUT_HDR_FL__ << "All Versions: " ;
	for(auto &v : versions)
		std::cout << v << " ";
	std::cout << std::endl;

	if (!versions.size()) return ConfigurationVersion(); //return INVALID

	return *(versions.rbegin());
}

//==============================================================================
// find all configuration versions by configuration type
std::set<ConfigurationVersion> DatabaseConfigurationInterface::getVersions(const ConfigurationBase* configuration) const
noexcept
try
{
	auto ifc = db::ConfigurationInterface{default_dbprovider};
	auto result = ifc.template getVersions<decltype(configuration)>(configuration, default_entity);

	auto resultSet = std::set<ConfigurationVersion>{};
	for(std::string const& version:result)
		resultSet.insert(ConfigurationVersion(std::stol(version, 0, 10)));

	//	auto to_set = [](auto const& inputList)
	//	{
	//		auto resultSet = std::set<ConfigurationVersion>{};
	//		std::for_each(inputList.begin(), inputList.end(),
	//				[&resultSet](std::string const& version)
	//				{ resultSet.insert(std::stol(version, 0, 10)); });
	//		return resultSet;
	//	};

	//auto vs = to_set(result);
	//for(auto &v:vs)
	//	std::cout << __COUT_HDR_FL__ << "\tversion " << v << std::endl;

	return resultSet;//to_set(result);
}
catch (std::exception const& e)
{
	std::cout << __COUT_HDR_FL__ << "DBI Exception:" << e.what() << "\n";
	return {};
}


//==============================================================================
// returns a list of all configuration names
std::set<std::string /*name*/> DatabaseConfigurationInterface::getAllConfigurationNames() const
throw(std::runtime_error)
try
{
	auto ifc = db::ConfigurationInterface{default_dbprovider};

	auto collection_name_prefix=std::string{};

	return ifc.listCollections(collection_name_prefix);
}
catch (std::exception const& e)
{
	__SS__ << "DBI Exception:" << e.what() << "\n";
	__COUT_ERR__ << ss.str();
	throw std::runtime_error(ss.str());
}
catch (...)
{
	__SS__ << "DBI Unknown exception.\n";
	__COUT_ERR__ << ss.str();
	throw std::runtime_error(ss.str());
}

//==============================================================================
// find all configuration groups in database
std::set<std::string /*name*/> DatabaseConfigurationInterface::getAllConfigurationGroupNames(const std::string &filterString) const
throw(std::runtime_error) try
{
	auto ifc = db::ConfigurationInterface{default_dbprovider};

	if(filterString == "")
		return ifc.findGlobalConfigurations("*"); //GConfig will return all GConfig* with filesystem db.. for mongodb would require reg expr
	else
		return ifc.findGlobalConfigurations(filterString + "*"); //GConfig will return all GConfig* with filesystem db.. for mongodb would require reg expr
}
catch (std::exception const& e)
{
	__SS__ << "DBI Exception:" << e.what() << "\n";
	__COUT_ERR__ << ss.str();
	throw std::runtime_error(ss.str());
}
catch (...)
{
	__SS__ << "DBI Unknown exception.\n";
	__COUT_ERR__ << ss.str();
	throw std::runtime_error(ss.str());
}


//==============================================================================
// find all configuration groups in database
std::set<ConfigurationGroupKey /*key*/> DatabaseConfigurationInterface::getKeys(const std::string &groupName) const
{
	std::set<ConfigurationGroupKey> retSet;
	std::set<std::string /*name*/> names = getAllConfigurationGroupNames();
	for(auto &n : names)
		if(n.find(groupName) == 0)
			retSet.insert(ConfigurationGroupKey(n));
	return retSet;
}

//==============================================================================
// return the contents of a configuration group
config_version_map_t DatabaseConfigurationInterface::getConfigurationGroupMembers(
		std::string const& configurationGroup, bool includeMetaDataTable) const
throw(std::runtime_error)
try
{
	//std::cout << __COUT_HDR_FL__ << "configurationGroup:" << configurationGroup << "\n";
	auto ifc = db::ConfigurationInterface{default_dbprovider};
	auto result = ifc.loadGlobalConfiguration(configurationGroup);

	auto to_map = [](auto const& inputList, bool includeMetaDataTable) {
		auto resultMap = config_version_map_t{};

		std::for_each(inputList.begin(), inputList.end(),
				[&resultMap](auto const& info) { resultMap[info.configuration] = std::stol(info.version, 0, 10); });

		if(!includeMetaDataTable)
		{
			//remove special meta data table from member map
			auto metaTable =
					resultMap.find(GROUP_METADATA_TABLE_NAME);
			if(metaTable != resultMap.end())
				resultMap.erase(metaTable);
		}
		return resultMap;
	};

	return to_map(result,includeMetaDataTable);
}
catch (std::exception const& e)
{
	__SS__ << "DBI Exception:" << e.what() << "\n";
	__COUT_ERR__ << ss.str();
	throw std::runtime_error(ss.str());
}
catch (...)
{
	__SS__ << "DBI Unknown exception.\n";
	__COUT_ERR__ << ss.str();
	throw std::runtime_error(ss.str());
}

//==============================================================================
// create a new configuration group from the contents map
void DatabaseConfigurationInterface::saveConfigurationGroup(config_version_map_t const& configurationMap,
		std::string const& configurationGroup) const
throw(std::runtime_error)
try
{
	auto ifc = db::ConfigurationInterface{default_dbprovider};

	auto to_list = [](auto const& inputMap) {
		auto resultList = VersionInfoList_t{};
		std::transform(inputMap.begin(), inputMap.end(), std::back_inserter(resultList), [](auto const& mapEntry) {
			return VersionInfoList_t::value_type{mapEntry.first, mapEntry.second.toString(), default_entity};
		});

		return resultList;
	};

	auto result = ifc.storeGlobalConfiguration(to_list(configurationMap), configurationGroup);

	if (result.first) return;

	throw std::runtime_error(result.second);
}
catch (std::exception const& e)
{
	__SS__ << "DBI Exception:" << e.what() << "\n";
	__COUT_ERR__ << ss.str();
	throw std::runtime_error(ss.str());
}
catch (...)
{
	__SS__ << "DBI Unknown exception.\n";
	__COUT_ERR__ << ss.str();
	throw std::runtime_error(ss.str());
}


