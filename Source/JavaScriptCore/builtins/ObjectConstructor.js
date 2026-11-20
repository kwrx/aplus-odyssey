/*
 * Copyright (C) 2016 Oleksandr Skachkov <gskachkov@gmail.com>.
 * Copyright (C) 2015 Jordan Harband. All rights reserved.
 * Copyright (C) 2018 Yusuke Suzuki <yusukesuzuki@slowstart.org>.
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

function fromEntries(iterable)
{
    "use strict";

    var object = {};

    for (var entry of iterable) {
        if (!@isObject(entry))
            @throwTypeError("Object.fromEntries requires the first iterable parameter yields objects");
        var key = entry[0];
        var value = entry[1];
        @putByValDirect(object, key, value);
    }

    return object;
}

function groupBy(items, callback)
{
    "use strict";

    if (@isUndefinedOrNull(items))
        @throwTypeError("Object.groupBy requires that the first argument not be null or undefined");
    if (!@isCallable(callback))
        @throwTypeError("Object.groupBy requires that the second argument be a function");

    var groups = @Object.@create(null);
    var k = 0;
    for (var value of items) {
        var key = callback.@call(@undefined, value, k++);
        if (typeof key !== "symbol")
            key = @toString(key);
        var list = groups[key];
        if (list === @undefined) {
            list = [];
            @putByValDirect(groups, key, list);
        }
        @putByValDirect(list, list.length, value);
    }
    return groups;
}

@overriddenName="groupBy"
function mapGroupBy(items, callback)
{
    "use strict";

    if (@isUndefinedOrNull(items))
        @throwTypeError("Map.groupBy requires that the first argument not be null or undefined");
    if (!@isCallable(callback))
        @throwTypeError("Map.groupBy requires that the second argument be a function");

    var groups = new @Map;
    var k = 0;
    for (var value of items) {
        var key = callback.@call(@undefined, value, k++);
        if (key === 0)
            key = 0;
        var list = groups.@get(key);
        if (list === @undefined) {
            list = [];
            groups.@set(key, list);
        }
        @putByValDirect(list, list.length, value);
    }
    return groups;
}
