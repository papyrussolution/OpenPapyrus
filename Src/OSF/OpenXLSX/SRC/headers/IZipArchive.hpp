// OpenXLSX
// Copyright (c) 2022, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_IZIPARCHIVE_HPP
#define OPENXLSX_IZIPARCHIVE_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <memory>
#include <string>

namespace OpenXLSX {
/**
 * @brief This class functions as a wrapper around any class that provides the necessary functionality for
 * a zip archive.
 * @details This class works by applying 'type erasure'. This enables the use of objects of any class, the only
 * requirement being that it provides the right interface. No inheritance from a base class is needed.
 */
class IZipArchive {
public:
	IZipArchive() : m_zipArchive() 
	{
	}
	/**
	 * @brief Constructor, taking the target object as an argument.
	 * @tparam T The type of the target object (will be auto deducted)
	 * @param x The target object
	 * @note This method is deliberately not marked 'explicit', because as a templated constructor, it should be able
	 * to take any type as an argument. However, only objects that satisfy the required interface can be used.
	 */
	template <typename T> IZipArchive(const T& zipArchive) : m_zipArchive{std::make_unique<Model<T> >(zipArchive)} 
	{
	}
	IZipArchive(const IZipArchive& other) : m_zipArchive(other.m_zipArchive ? other.m_zipArchive->clone() : nullptr) 
	{
	}
	IZipArchive(IZipArchive && other) noexcept = default;
	~IZipArchive() = default;
	template <typename T> inline IZipArchive& operator=(const T& zipArchive)
	{
		m_zipArchive = std::make_unique<Model<T> >(zipArchive);
		return *this;
	}
	inline IZipArchive& operator=(const IZipArchive& other)
	{
		IZipArchive copy(other);
		*this = std::move(copy);
		return *this;
	}
	inline IZipArchive& operator=(IZipArchive&& other) noexcept = default;
	inline explicit operator bool() const { return isValid(); }
	inline bool isValid() const { return m_zipArchive->isValid(); }
	inline bool isOpen() const { return m_zipArchive->isOpen(); }
	inline void open(const std::string& fileName) { m_zipArchive->open(fileName); }
	inline void close() const { m_zipArchive->close(); }
	inline void save(const std::string& path) { m_zipArchive->save(path); }
	inline void addEntry(const std::string& name, const std::string& data) { m_zipArchive->addEntry(name, data); }
	inline void deleteEntry(const std::string& entryName) { m_zipArchive->deleteEntry(entryName); }
	inline std::string getEntry(const std::string& name) { return m_zipArchive->getEntry(name); }
	inline bool hasEntry(const std::string& entryName) const { return m_zipArchive->hasEntry(entryName); }
private:
	struct Concept {
	public:
		Concept() = default;
		Concept(const Concept&) = default;
		Concept(Concept&&) noexcept = default;
		virtual ~Concept() = default;
		inline Concept& operator=(const Concept&) = default;
		inline Concept& operator=(Concept&&) noexcept = default;
		inline virtual std::unique_ptr<Concept> clone() const = 0;
		inline virtual bool isValid() const = 0;
		inline virtual bool isOpen() const = 0;
		inline virtual void open(const std::string& fileName) = 0;
		inline virtual void close() = 0;
		inline virtual void save(const std::string& path) = 0;
		inline virtual void addEntry(const std::string& name, const std::string& data) = 0;
		inline virtual void deleteEntry(const std::string& entryName) = 0;
		inline virtual std::string getEntry(const std::string& name) = 0;
		inline virtual bool hasEntry(const std::string& entryName) const = 0;
	};

	template <typename T> struct Model : Concept {
	public:
		explicit Model(const T& x) : ZipType(x) 
		{
		}
		Model(const Model& other) = default;
		Model(Model&& other) noexcept = default;
		~Model() override = default;
		inline Model& operator=(const Model& other) = default;
		inline Model& operator=(Model&& other) noexcept = default;
		inline std::unique_ptr<Concept> clone() const override { return std::make_unique<Model<T> >(ZipType); }
		inline bool isValid() const override { return ZipType.isValid(); }
		inline bool isOpen() const override { return ZipType.isOpen(); }
		inline void open(const std::string& fileName) override { ZipType.open(fileName); }
		inline void close() override { ZipType.close(); }
		inline void save(const std::string& path) override { ZipType.save(path); }
		inline void addEntry(const std::string& name, const std::string& data) override { ZipType.addEntry(name, data); }
		inline void deleteEntry(const std::string& entryName) override { ZipType.deleteEntry(entryName); }
		inline std::string getEntry(const std::string& name) override { return ZipType.getEntry(name); }
		inline bool hasEntry(const std::string& entryName) const override { return ZipType.hasEntry(entryName); }
private:
		T ZipType;
	};

	std::unique_ptr<Concept> m_zipArchive;
};
}

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_IZIPARCHIVE_HPP
