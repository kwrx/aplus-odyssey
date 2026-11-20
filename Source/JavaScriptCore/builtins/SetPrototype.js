/*
 * Copyright (C) 2016 Yusuke Suzuki <utatane.tea@gmail.com>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

function forEach(callback /*, thisArg */)
{
    "use strict";

    if (!@isSet(this))
        @throwTypeError("Set operation called on non-Set object");

    if (!@isCallable(callback))
        @throwTypeError("Set.prototype.forEach callback must be a function");

    var thisArg = @argument(1);
    var bucket = @setBucketHead(this);

    do {
        bucket = @setBucketNext(bucket);
        if (bucket === @sentinelSetBucket)
            break;
        var key = @setBucketKey(bucket);
        callback.@call(thisArg, key, key, this);
    } while (true);
}

@globalPrivate
function getSetRecord(other)
{
    "use strict";

    if (!@isObject(other))
        @throwTypeError("Set operation expects first argument to be an object");

    var size = @toNumber(other.size);
    if (size !== size)
        @throwTypeError("Set operation expects first argument to have a numeric 'size' property");
    if (size < 0)
        @throwRangeError("Set operation expects first argument to have a non-negative 'size' property");

    var has = other.has;
    if (!@isCallable(has))
        @throwTypeError("Set operation expects first argument to have a callable 'has' method");

    var keys = other.keys;
    if (!@isCallable(keys))
        @throwTypeError("Set operation expects first argument to have a callable 'keys' method");

    return { size: size, has: has, keys: keys };
}

@globalPrivate
function setRecordKeys(other, record)
{
    "use strict";

    var iterator = record.keys.@call(other);
    if (!@isObject(iterator))
        @throwTypeError("Set operation expects 'keys' to return an object");
    var next = iterator.next;
    var values = [];
    while (true) {
        var step = next.@call(iterator);
        if (!@isObject(step))
            @throwTypeError("Iterator result is not an object");
        if (step.done)
            break;
        var value = step.value;
        if (value === 0)
            value = 0;
        @putByValDirect(values, values.length, value);
    }
    return values;
}

@globalPrivate
function setValues(set)
{
    "use strict";

    var values = [];
    var bucket = @setBucketHead(set);
    while (true) {
        bucket = @setBucketNext(bucket);
        if (bucket === @sentinelSetBucket)
            break;
        @putByValDirect(values, values.length, @setBucketKey(bucket));
    }
    return values;
}

function union(other)
{
    "use strict";

    if (!@isSet(this))
        @throwTypeError("Set.prototype.union called on non-Set object");

    var record = @getSetRecord(other);
    var result = new @Set;
    var values = @setValues(this);
    for (var i = 0; i < values.length; ++i)
        result.@add(values[i]);
    var otherValues = @setRecordKeys(other, record);
    for (var i = 0; i < otherValues.length; ++i)
        result.@add(otherValues[i]);
    return result;
}

function intersection(other)
{
    "use strict";

    if (!@isSet(this))
        @throwTypeError("Set.prototype.intersection called on non-Set object");

    var record = @getSetRecord(other);
    var result = new @Set;
    if (this.size <= record.size) {
        var values = @setValues(this);
        for (var i = 0; i < values.length; ++i) {
            if (record.has.@call(other, values[i]))
                result.@add(values[i]);
        }
    } else {
        var otherValues = @setRecordKeys(other, record);
        for (var i = 0; i < otherValues.length; ++i) {
            if (this.@has(otherValues[i]))
                result.@add(otherValues[i]);
        }
    }
    return result;
}

function difference(other)
{
    "use strict";

    if (!@isSet(this))
        @throwTypeError("Set.prototype.difference called on non-Set object");

    var record = @getSetRecord(other);
    var result = new @Set;
    var values = @setValues(this);
    if (this.size <= record.size) {
        for (var i = 0; i < values.length; ++i) {
            if (!record.has.@call(other, values[i]))
                result.@add(values[i]);
        }
    } else {
        var excluded = new @Set;
        var otherValues = @setRecordKeys(other, record);
        for (var i = 0; i < otherValues.length; ++i)
            excluded.@add(otherValues[i]);
        for (var i = 0; i < values.length; ++i) {
            if (!excluded.@has(values[i]))
                result.@add(values[i]);
        }
    }
    return result;
}

function symmetricDifference(other)
{
    "use strict";

    if (!@isSet(this))
        @throwTypeError("Set.prototype.symmetricDifference called on non-Set object");

    var record = @getSetRecord(other);
    var otherSet = new @Set;
    var otherValues = @setRecordKeys(other, record);
    for (var i = 0; i < otherValues.length; ++i)
        otherSet.@add(otherValues[i]);

    var result = new @Set;
    var values = @setValues(this);
    for (var i = 0; i < values.length; ++i) {
        if (!otherSet.@has(values[i]))
            result.@add(values[i]);
    }
    var otherUnique = @setValues(otherSet);
    for (var i = 0; i < otherUnique.length; ++i) {
        if (!this.@has(otherUnique[i]))
            result.@add(otherUnique[i]);
    }
    return result;
}

function isSubsetOf(other)
{
    "use strict";

    if (!@isSet(this))
        @throwTypeError("Set.prototype.isSubsetOf called on non-Set object");

    var record = @getSetRecord(other);
    if (this.size > record.size)
        return false;
    var values = @setValues(this);
    for (var i = 0; i < values.length; ++i) {
        if (!record.has.@call(other, values[i]))
            return false;
    }
    return true;
}

function isSupersetOf(other)
{
    "use strict";

    if (!@isSet(this))
        @throwTypeError("Set.prototype.isSupersetOf called on non-Set object");

    var record = @getSetRecord(other);
    if (this.size < record.size)
        return false;
    var otherValues = @setRecordKeys(other, record);
    for (var i = 0; i < otherValues.length; ++i) {
        if (!this.@has(otherValues[i]))
            return false;
    }
    return true;
}

function isDisjointFrom(other)
{
    "use strict";

    if (!@isSet(this))
        @throwTypeError("Set.prototype.isDisjointFrom called on non-Set object");

    var record = @getSetRecord(other);
    if (this.size <= record.size) {
        var values = @setValues(this);
        for (var i = 0; i < values.length; ++i) {
            if (record.has.@call(other, values[i]))
                return false;
        }
    } else {
        var otherValues = @setRecordKeys(other, record);
        for (var i = 0; i < otherValues.length; ++i) {
            if (this.@has(otherValues[i]))
                return false;
        }
    }
    return true;
}
