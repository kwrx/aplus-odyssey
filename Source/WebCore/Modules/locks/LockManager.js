/*
 * Copyright (C) 2026 Antonino Natale
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

function request(name, options)
{
    "use strict";

    const manager = this;
    if (!@isLockManager(manager))
        return @Promise.@reject(@makeThisTypeError("LockManager", "request"));

    let callback;
    if (arguments.length < 3) {
        callback = options;
        options = @undefined;
    } else
        callback = arguments[2];

    let mode = "exclusive";
    let ifAvailable = false;
    let steal = false;
    let signal = @undefined;

    try {
        if (arguments.length < 2)
            @throwTypeError("Not enough arguments");

        name = `${name}`;

        if (options !== @undefined && options !== null) {
            if (!@isObject(options))
                @throwTypeError("The options argument must be an object");
            ifAvailable = !!options.ifAvailable;
            const optionMode = options.mode;
            if (optionMode !== @undefined)
                mode = `${optionMode}`;
            signal = options.signal;
            steal = !!options.steal;
        }

        if (mode !== "exclusive" && mode !== "shared")
            @throwTypeError("The provided mode is not a valid LockMode");
        if (signal !== @undefined && !@isAbortSignal(signal))
            @throwTypeError("The signal member must be an AbortSignal");
        if (typeof callback !== "function")
            @throwTypeError("The callback argument must be a function");
    } catch (e) {
        return @Promise.@reject(e);
    }

    if (name[0] === "-")
        return @Promise.@reject(@makeDOMException("NotSupportedError", "Lock names starting with '-' are reserved"));
    if (steal && ifAvailable)
        return @Promise.@reject(@makeDOMException("NotSupportedError", "The 'steal' and 'ifAvailable' options cannot be used together"));
    if (steal && mode !== "exclusive")
        return @Promise.@reject(@makeDOMException("NotSupportedError", "The 'steal' option may only be used with exclusive locks"));
    if (signal !== @undefined && (steal || ifAvailable))
        return @Promise.@reject(@makeDOMException("NotSupportedError", "The 'signal' option cannot be used with 'steal' or 'ifAvailable'"));
    if (signal !== @undefined && signal.aborted)
        return @Promise.@reject(@makeDOMException("AbortError", "The lock request was aborted"));

    const id = manager.@newLockRequestId();
    const result = @newPromiseCapability(@Promise);
    let state = 0;

    const settle = (succeeded, value) => {
        manager.@releaseLock(id);
        if (succeeded)
            result.@resolve.@call(@undefined, value);
        else
            result.@reject.@call(@undefined, value);
    };

    if (signal !== @undefined) {
        @whenSignalAborted(signal, () => {
            if (state !== 0)
                return;
            state = 2;
            settle(false, @makeDOMException("AbortError", "The lock request was aborted"));
        });
    }

    manager.@requestLock(id, name, mode === "exclusive", ifAvailable, steal).@then((granted) => {
        if (state !== 0) {
            manager.@releaseLock(id);
            return;
        }
        state = 1;

        let value;
        try {
            value = callback(granted ? { name, mode } : null);
        } catch (e) {
            settle(false, e);
            return;
        }

        @Promise.@resolve(value).@then((v) => settle(true, v), (e) => settle(false, e));
    }, (e) => {
        if (state !== 0)
            return;
        state = 2;
        settle(false, e);
    });

    return result.@promise;
}

function query()
{
    "use strict";

    if (!@isLockManager(this))
        return @Promise.@reject(@makeThisTypeError("LockManager", "query"));

    const snapshot = this.@lockSnapshot();
    const held = [];
    const pending = [];

    for (let i = 0; i + 3 < snapshot.length; i += 4) {
        const info = { name: snapshot[i + 1], mode: snapshot[i + 2], clientId: snapshot[i + 3] };
        if (snapshot[i] === "held")
            @putByValDirect(held, held.length, info);
        else
            @putByValDirect(pending, pending.length, info);
    }

    return @Promise.@resolve({ held, pending });
}
