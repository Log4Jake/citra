// Copyright 2016 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "config/config_info.h"
#include "config/string_util.h"

namespace Config {
namespace detail {
template <typename T, std::enable_if_t<!std::is_enum<T>::value>* = nullptr>
std::optional<T> TryParse(const std::string& str_value) {
    T value;
    if (!::TryParse(str_value, &value))
        return std::nullopt;
    return value;
}

template <typename T, std::enable_if_t<std::is_enum<T>::value>* = nullptr>
std::optional<T> TryParse(const std::string& str_value) {
    const auto result = TryParse<std::underlying_type_t<T>>(str_value);
    if (result)
        return static_cast<T>(*result);
    return {};
}

template <>
inline std::optional<std::string> TryParse(const std::string& str_value) {
    return str_value;
}
} // namespace detail

template <typename T>
struct ConfigInfo;

class Layer;
using LayerMap = std::map<ConfigLocation, std::optional<std::string>>;

class ConfigLayerLoader {
public:
    explicit ConfigLayerLoader();
    virtual ~ConfigLayerLoader();
    virtual void Load(Layer* config_layer) = 0;
    virtual void Save(Layer* config_layer) = 0;
};

class Section {
public:
    using iterator = LayerMap::iterator;
    Section(iterator begin_, iterator end_) : m_begin(begin_), m_end(end_) {}
    iterator begin() const {
        return m_begin;
    }
    iterator end() const {
        return m_end;
    }

private:
    iterator m_begin;
    iterator m_end;
};

class ConstSection {
public:
    using iterator = LayerMap::const_iterator;
    ConstSection(iterator begin_, iterator end_) : m_begin(begin_), m_end(end_) {}
    iterator begin() const {
        return m_begin;
    }
    iterator end() const {
        return m_end;
    }

private:
    iterator m_begin;
    iterator m_end;
};

class Layer {
public:
    explicit Layer(std::unique_ptr<ConfigLayerLoader> loader);
    virtual ~Layer();

    // Convenience functions
    bool Exists(const ConfigLocation& location) const;
    bool DeleteKey(const ConfigLocation& location);
    void DeleteAllKeys();

    template <typename T>
    T Get(const ConfigInfo<T>& config_info) const {
        return Get<T>(config_info.location).value_or(config_info.default_value);
    }

    template <typename T>
    std::optional<T> Get(const ConfigLocation& location) const {
        auto iter = m_map.find(location);
        if (iter == m_map.end() || !iter->second) {
            return std::nullopt;
        }
        return detail::TryParse<T>(*iter->second);
    }

    template <typename T>
    void Set(const ConfigInfo<T>& config_info, const std::common_type_t<T>& value) {
        if (config_info.default_value == value) {
            auto iter = m_map.find(config_info.location);
            if (iter != m_map.end() && iter->second) {
                iter->second.reset();
                m_dirty = true;
            }
            return;
        }
        Set(config_info.location, value);
    }

    template <typename T>
    void Set(const ConfigLocation& location, const T& value) {
        Set(location, ValueToString(value));
    }

    void Set(const ConfigLocation& location, const std::string& new_value) {
        auto& current_value = m_map[location];
        if (current_value != new_value) {
            current_value = new_value;
            m_dirty = true;
        }
    }

    Section GetSection(const std::string& section);
    ConstSection GetSection(const std::string& section) const;

    // Explicit load and save of layers
    void Load();
    void Save();
    void Clear();

    const LayerMap& GetLayerMap() const;

protected:
    bool m_dirty = false;
    LayerMap m_map;
    std::unique_ptr<ConfigLayerLoader> m_loader;
};
} // namespace Config
