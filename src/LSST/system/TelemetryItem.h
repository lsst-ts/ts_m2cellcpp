/*
 *  This file is part of LSST M2 support system package.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */
#ifndef LSST_M2CELLCPP_SYSTEM_TELEMETRYITEM_H
#define LSST_M2CELLCPP_SYSTEM_TELEMETRYITEM_H

// System headers
#include <arpa/inet.h>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

// Third party headers
#include "nlohmann/json.hpp"

// project headers
#include "util/Issue.h"
#include "util/Log.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

/// Exception specific to Telemetry.
///
/// unit test: test_TelemetryCom.cpp
class TelemetryException : public util::Issue {
public:
    TelemetryException(Context const& ctx, std::string const& msg) : util::Issue(ctx, msg) {}
};


class TelemetryItem;

/// Type the `TelemetryItemMap`. Objects of this will be copied, so the
/// map must store pointers.
typedef std::map<std::string, std::shared_ptr<TelemetryItem>> TelemetryItemMap;
typedef std::shared_ptr<TelemetryItemMap> TelemetryItemMapPtr;

/// One item that can be sent by the server in `TelemetryCom`. This class
/// can be used to build items with several elements.
/// @see `TItemPowerStatusBase` and `TItemDouble` as examples.
/// as an example.
/// Unit tests in tests/test_TelemetryCom
class TelemetryItem {
public:
    using Ptr = std::shared_ptr<TelemetryItem>;

    /// Return false if `tiMap` is not null and item` was not successfully inserted into `tiMap`.
    /// Insert `item` into `tiMap`.
    /// @throw Telemetry
    static void insert(TelemetryItemMap* tiMap, Ptr const& item);

    /// Return true if all elements of `mapA` have a match with the sames values in `mapB` and both
    /// maps are the same size.
    /// @param mapA - arbitray `TelemetryItemMap` to be compared to `mapB`
    /// @param mapB - arbitray `TelemetryItemMap` to be compared to `mapA`
    /// @param note - optional, used to help identify why the two maps are being compared in the log.
    /// NOTE: Comparing a map to itself will likely deadlock.
    static bool compareTelemetryItemMaps(TelemetryItemMap const& mapA, TelemetryItemMap const& mapB, std::string const& note ="");

    /// Create a `TelemetryItem` with immutable id of `id`.
    TelemetryItem(std::string const& id) : _id(id) {}

    virtual ~TelemetryItem() = default;

    TelemetryItem() = delete;
    TelemetryItem(TelemetryItem const&) = delete;
    TelemetryItem& operator=(TelemetryItem const&) = delete;

    /// Return the id string
    std::string getId() const { return _id; };

    /// Return a json object containing the id.
    virtual nlohmann::json getJson() const {
        return buildJsonFromMap(_tiMap);
    }

    /// Return true if this item and `other` have the same id and values.
    virtual bool compareItem(TelemetryItem const& other) const = 0;

    /// Parse the json string `jStr` and load the appropriate values into this object.
    /// @return true if item parsed correctly and had appropriate data.
    bool parse(std::string const& jStr);

    /// Set the value of this `TelemetryItem` from `js`.
    /// @param idExpected - If set to true, `js` must contain a valid entry for `id`.
    /// @return true if the value of all relate items could be set from `js`
    virtual bool setFromJson(nlohmann::json const& js, bool idExpected = false) {
        return setMapFromJson(_tiMap, js, idExpected);
    }

    /// Return `_doNotSend`.
    bool getDoNotSend() { return _doNotSend; }

    /// Set `_doNotSend` to `val`.
    void setDoNotSend(bool val) { _doNotSend = val; }

    /// Return a string reasonable for logging.
    std::string dump() const {
        std::stringstream os;
        os << getJson();
        return os.str();
    }

    /// Add a reasonable string representation of `item` for logging to `os`
    friend std::ostream& operator<<(std::ostream& os, TelemetryItem const& item);

protected:
    /// Build a json object using this object and the provided `tMap`.
    nlohmann::json buildJsonFromMap(TelemetryItemMap const& tMap) const;

    /// Return false if values in `tMap` cannot be set from `js` and `idExpected`.
    bool setMapFromJson(TelemetryItemMap& tMap, nlohmann::json const& js, bool idExpected);

    /// Return false if `idExpected` and the `js` "id" entry is wrong.
    bool checkIdCorrect(nlohmann::json const& js, bool idExpected) const;

    /// Return true if all items in `telemItemA` and `telemItemB` match.
    template <class TIC>
    static bool compareItemsTemplate(TelemetryItem const& telemItemA, TelemetryItem const& telemItemB) {
        try {
            TIC const& aItem = dynamic_cast<TIC const&>(telemItemA);
            TIC const& bItem = dynamic_cast<TIC const&>(telemItemB);
            return compareTelemetryItemMaps(aItem._tiMap, bItem._tiMap);
        } catch (std::bad_cast const& ex) {
            return false;
        }
    }

    /// Map of items for this TelemetryItem. Does not change after constructor.
    TelemetryItemMap _tiMap;
private:
    std::string const _id; ///< Identifier for this item.
    std::atomic<bool> _doNotSend{false}; ///< Do not transmit this item when this is true.
};

/// This is a TelemetryItem child class for handling simple single values (such as
/// double, boolean, int, etc.), primarily for the purpose of reading them
/// in and out of json objects. This class only supports types supported by
/// std::atomic.
/// Unit tests in tests/test_TelemetryCom
template <class ST>
class TItemSimple : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemSimple>;

    TItemSimple() = delete;

    /// Create a `TItemSimple` object with identifier `id` and option value `defaultVal` for all entries.
    TItemSimple(std::string const& id, ST defaultVal = 0) : TelemetryItem(id), _val(defaultVal) {}

    ~TItemSimple() override {}

    /// Return a copy of the value.
    ST getVal() const {
        ST ret = _val;
        return ret;
    }

    /// Set the value of the object to `val`.
    void setVal(ST const& val) { _val = val; }

    /// Return the json representation of this object.
    nlohmann::json getJson() const override {
        nlohmann::json js;
        ST v = _val;  // conversion from atomic fails when js[getId()] = _val;
        js[getId()] = v;
        return js;
    }

    /// Set the value of this object from json.
    bool setFromJson(nlohmann::json const& js, bool idExpected) {
        if (idExpected) {
            // This type can only have a simple type value for `_val`.
            LERROR("TItemSimple::setFromJson cannot have a json 'id' entry");
            return false;
        }
        try {
            ST val = js.at(getId());
            setVal(val);
            return true;
        } catch (nlohmann::json::out_of_range const& ex) {
            LERROR("TItemSimple::setFromJson out of range for ", getId(), " js=", js);
        }
        return false;
    }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        try {
            TItemSimple<ST> const& otherItem = dynamic_cast<TItemSimple<ST> const&>(other);
            return (getId() == otherItem.getId() && _val == otherItem._val);
        } catch (std::bad_cast const& ex) {
            return false;
        }
    }

private:
    std::atomic<ST> _val; ///< Stores the typed value for this item.
};


/// This class is used to store a specific telemetry item with a double value.
/// Unit tests in tests/test_TelemetryCom
class TItemDouble : public TItemSimple<double> {
public:
    using Ptr = std::shared_ptr<TItemDouble>;

    /// Create a shared pointer instance of TItemDouble using `id`, and `defaultVal`, and
    /// insert it into the map `itMap`.
    /// @throws TelemetryException if the new object cannot be inserted into the list.
    static Ptr create(std::string const& id, TelemetryItemMap* tiMap, double defaultVal = 0.0) {
        Ptr newItem = Ptr(new TItemDouble(id, defaultVal));
        insert(tiMap, newItem);
        return newItem;
    }

    /// Create a `TItemDouble` object with identifier `id` and option value `defaultVal`.
    TItemDouble(std::string const& id, double defaultVal = 0.0) : TItemSimple(id, defaultVal) {}

    TItemDouble() = delete;

    virtual ~TItemDouble() {}
};

/// This class is used to store a specific telemetry item with a bool value.
/// Unit tests in tests/test_TelemetryCom
class TItemBoolean : public TItemSimple<bool> {
public:
    using Ptr = std::shared_ptr<TItemBoolean>;

    /// Create a shared pointer instance of TItemBoolean using `id`, and `defaultVal`, and
    /// insert it into the map `itMap`.
    /// @throws TelemetryException if the new object cannot be inserted into the list.
    static Ptr create(std::string const& id, TelemetryItemMap* tiMap, bool defaultVal = false) {
        Ptr newItem = Ptr(new TItemBoolean(id, defaultVal));
        insert(tiMap, newItem);
        return newItem;
    }

    /// Create a `TItemBoolean` object with identifier `id` and option value `defaultVal`.
    TItemBoolean(std::string const& id, double defaultVal = 0.0) : TItemSimple(id, defaultVal) {}

    TItemBoolean() = delete;

    virtual ~TItemBoolean() {}
};

/// This class is used to store a specific telemetry item with a string value.
/// Unit tests in tests/test_TelemetryCom
class TItemString : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemString>;

    TItemString() = delete;

    /// Create a shared pointer instance of TItemString using `id`, and `defaultVal`, and
    /// insert it into the map `itMap`.
    /// @throws TelemetryException if the new object cannot be inserted into the list.
    static Ptr create(std::string const& id, TelemetryItemMap* tiMap, std::string const& defaultVal = std::string()) {
        Ptr newItem = Ptr(new TItemString(id, defaultVal));
        insert(tiMap, newItem);
        return newItem;
    }

    /// Create a `TItemString` object with identifier `id` and option value `defaultVal` for all entries.
    TItemString(std::string const& id, std::string const& defaultVal = std::string()) : TelemetryItem(id), _val(defaultVal) {}

    ~TItemString() override {}

    /// Return a copy of the value.
    std::string getVal() const {
    	std::lock_guard<std::mutex> lg(_stMtx);
    	return _val;
    }

    /// Set the value of the object to `val`.
    void setVal(std::string const& val) {
    	std::lock_guard<std::mutex> lg(_stMtx);
    	_val = val;
    }

    /// Return the json representation of this object.
    nlohmann::json getJson() const override {
        nlohmann::json js;
        std::lock_guard<std::mutex> lg(_stMtx);
        js[getId()] = _val;
        return js;
    }

    /// Set the value of this object from json.
    bool setFromJson(nlohmann::json const& js, bool idExpected) {
        if (idExpected) {
            // This type can only have a simple type value for `_val`.
            LERROR("TItemSimple::setFromJson cannot have a json 'id' entry");
            return false;
        }
        try {
            std::string val = js.at(getId());
            setVal(val);
            return true;
        } catch (nlohmann::json::out_of_range const& ex) {
            LERROR("TItemSimple::setFromJson out of range for ", getId(), " js=", js);
        }
        return false;
    }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        try {
            TItemString const& otherItem = dynamic_cast<TItemString const&>(other);
            // There's no way to reliably control which gets locked first
            // (both A.compareItem(B) or B.compareItem(A) are possible),
            // so this is needed to lock both without risk of deadlock.
            std::unique_lock lockThis(_stMtx, std::defer_lock);
            std::unique_lock lockOther(otherItem._stMtx, std::defer_lock);
            std::lock(lockThis, lockOther);
            return (getId() == otherItem.getId() && _val == otherItem._val);
        } catch (std::bad_cast const& ex) {
            return false;
        }
    }

private:
    std::string _val; ///< Stores the typed value for this item.
    mutable std::mutex _stMtx; ///< Protects `_val`.
};


/// This is a TelemetryItem child class for handling vectors, primarily
/// for the purpose of reading them in and out of json objects.
/// Unit tests in tests/test_TelemetryCom
template <class VT>
class TItemVector : public TelemetryItem {
public:
    using Ptr = std::shared_ptr<TItemVector>;

    TItemVector() = delete;

    /// Create a `TItemVector` object with identifier `id` and option value `defaultVal` for all entries.
    TItemVector(std::string const& id, int size, VT defaultVal = 0) : TelemetryItem(id), _size(size) {
        for(size_t j=0; j<_size; ++j) {
            _vals.push_back(defaultVal);
        }
    }

    ~TItemVector() override {}

    /// Return a vector copy of the values in this object.
    std::vector<VT> getVals() const {
        std::vector<VT> outVals;
        std::lock_guard<std::mutex> lg(_mtx);
        for (auto const& v : _vals) {
            outVals.push_back(v);
        }
        return outVals;
    }

    /// Set the value of the object to `vals`, which must have the same size as `_vals`.
    /// @return false if the size of `vals` is different from `_size`.
    bool setVals(std::vector<VT> const& vals) {
        if (vals.size() != _size) {
            LERROR("TItemVector::setVals wrong size vals.size()=", vals.size(), " for ", dump());
            return false;
        }
        std::lock_guard<std::mutex> lg(_mtx);
        for (size_t j = 0; j < _size; ++j) {
            _vals[j] = vals[j];
        }
        return true;
    }

    /// Set the value at `index` in `_vals` to val.
    /// @return false if index is out of range.
    bool setVal(size_t index, VT val) {
        if (index > _size) {
            return false;
        }
        std::lock_guard<std::mutex> lg(_mtx);
        _vals[index] = val;
        return true;
    }

    /// Return the value at `index` in `_vals`.
    /// @throw TelemetryException if index is out of range.
    VT getVal(size_t index) const {
        if (index > _size) {
            throw TelemetryException(ERR_LOC, "TItemVector::getVal out of range for index=" +
                                     std::to_string(index) + " for " + dump());
        }
        std::lock_guard<std::mutex> lg(_mtx);
        return _vals[index];
    }

    /// Return the json representation of this object.
    nlohmann::json getJson() const override {
        nlohmann::json js;
        nlohmann::json jArray = nlohmann::json::array();

        std::lock_guard<std::mutex> lg(_mtx);
        for (size_t j = 0; j < _size; ++j) {
            jArray.push_back(_vals[j]);
        }
        js[getId()] = jArray;
        return js;
    }

    /// Set the value of this object from json.
    bool setFromJson(nlohmann::json const& js, bool idExpected) override {
        if (idExpected) {
            // This type can only have a typed value array for `_vals`.
            LERROR("TItemVector::setFromJson cannot have a json 'id' entry");
            return false;
        }
        try {
            std::vector<VT> vals = js.at(getId());
            return setVals(vals);
        } catch (nlohmann::json::out_of_range const& ex) {
            LERROR("TItemVector::setFromJson out of range for ", getId(), " js=", js);
        }
        return false;
    }

    /// Return true if this item and `other` have the same id and values.
    bool compareItem(TelemetryItem const& other) const override {
        try {
            TItemVector const& otherT = dynamic_cast<TItemVector const&>(other);
            if (getId() != other.getId() || _size != otherT._size) {
                return false;
            }
            // There's no way to reliably control which gets locked first
            // (both A.compareItem(B) or B.compareItem(A) are possible),
            // so this is needed to lock both without risk of deadlock.
            std::unique_lock lockThis(_mtx, std::defer_lock);
            std::unique_lock lockOther(otherT._mtx, std::defer_lock);
            std::lock(lockThis, lockOther);
            return (_vals == otherT._vals);
        } catch (std::bad_cast const& ex) {
            return false;
        }
    }

private:
    size_t const _size; ///< Number of elements in `_vals`.
    std::vector<VT> _vals; ///< Vector containing the typed values with length _size.
    mutable std::mutex _mtx; ///< Protects `_vals`.
};


/// This is a TItemVector child class for handling vectors of doubles.
/// Unit tests in tests/test_TelemetryCom
class TItemVectorDouble : public TItemVector<double> {
public:
    using Ptr = std::shared_ptr<TItemVectorDouble>;

    /// Create a shared pointer instance of TItemVectorDouble using `id`, number of elements `size, and `defaultVal` for
    /// all entries, then insert it into the map `itMap`.
    /// @throws TelemetryException if the new object cannot be inserted into the list
    static Ptr create(std::string const& id, size_t size, TelemetryItemMap* tiMap, double defaultVal = 0.0) {
        Ptr newItem = Ptr(new TItemVectorDouble(id, size, defaultVal));
        insert(tiMap, newItem);
        return newItem;
    }

    TItemVectorDouble() = delete;

    /// Create a `TItemDouble` object with identifier `id` and option value `defaultVal` for all entries.
    TItemVectorDouble(std::string const& id, int size, double defaultVal = 0.0) : TItemVector(id, size, defaultVal) {}

    ~TItemVectorDouble() override {}
};
/// This is a TItemVector child class for handling vectors of ints.
/// Unit tests in tests/test_TelemetryCom
class TItemVectorInt : public TItemVector<int> {
public:
    using Ptr = std::shared_ptr<TItemVectorInt>;

    /// Create a shared pointer instance of TItemVectorInt using `id`, number of elements `size, and `defaultVal` for
    /// all entries, then insert it into the map `itMap`.
    /// @throws TelemetryException if the new object cannot be inserted into the list
    static Ptr create(std::string const& id, size_t size, TelemetryItemMap* tiMap, int defaultVal = 0) {
        Ptr newItem = Ptr(new TItemVectorInt(id, size, defaultVal));
        insert(tiMap, newItem);
        return newItem;
    }
    TItemVectorInt() = delete;

    /// Create a `TItemDouble` object with identifier `id` and option value `defaultVal` for all entries.
    TItemVectorInt(std::string const& id, int size, int defaultVal = 0) : TItemVector(id, size, defaultVal) {}

    ~TItemVectorInt() override {}
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYITEM_H
