
// This file is part of node-lmdb, the Node.js binding for lmdb
// Copyright (c) 2013 Timur Kristóf
// Licensed to you under the terms of the MIT license
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "node-lmdb.h"

using namespace v8;
using namespace node;

TxnWrap::TxnWrap(MDB_env *env, MDB_txn *txn) {
    this->env = env;
    this->txn = txn;
}

TxnWrap::~TxnWrap() {
    // Close if not closed already
    if (this->txn) {
        mdb_txn_abort(txn);
        this->ew->Unref();
    }
}

NAN_METHOD(TxnWrap::ctor) {
    Nan::HandleScope scope;

    EnvWrap *ew = Nan::ObjectWrap::Unwrap<EnvWrap>(info[0]->ToObject());
    int flags = 0;

    if (info[1]->IsObject()) {
        Local<Object> options = info[1]->ToObject();

        // Get flags from options

        setFlagFromValue(&flags, MDB_RDONLY, "readOnly", false, options);
    }


    MDB_txn *txn;
    int rc = mdb_txn_begin(ew->env, nullptr, flags, &txn);
    if (rc != 0) {
        return Nan::ThrowError(mdb_strerror(rc));
    }

    TxnWrap* tw = new TxnWrap(ew->env, txn);
    tw->ew = ew;
    tw->ew->Ref();
    tw->Wrap(info.This());

    NanReturnThis();
}

NAN_METHOD(TxnWrap::commit) {
    Nan::HandleScope scope;

    TxnWrap *tw = Nan::ObjectWrap::Unwrap<TxnWrap>(info.This());

    if (!tw->txn) {
        return Nan::ThrowError("The transaction is already closed.");
    }

    int rc = mdb_txn_commit(tw->txn);
    tw->txn = nullptr;
    tw->ew->Unref();

    if (rc != 0) {
        return Nan::ThrowError(mdb_strerror(rc));
    }

    return;
}

NAN_METHOD(TxnWrap::abort) {
    Nan::HandleScope scope;

    TxnWrap *tw = Nan::ObjectWrap::Unwrap<TxnWrap>(info.This());

    if (!tw->txn) {
        return Nan::ThrowError("The transaction is already closed.");
    }

    mdb_txn_abort(tw->txn);
    tw->ew->Unref();
    tw->txn = nullptr;

    return;
}

NAN_METHOD(TxnWrap::reset) {
    Nan::HandleScope scope;

    TxnWrap *tw = Nan::ObjectWrap::Unwrap<TxnWrap>(info.This());

    if (!tw->txn) {
        return Nan::ThrowError("The transaction is already closed.");
    }

    mdb_txn_reset(tw->txn);

    return;
}

NAN_METHOD(TxnWrap::renew) {
    Nan::HandleScope scope;

    TxnWrap *tw = Nan::ObjectWrap::Unwrap<TxnWrap>(info.This());

    if (!tw->txn) {
        return Nan::ThrowError("The transaction is already closed.");
    }

    int rc = mdb_txn_renew(tw->txn);
    if (rc != 0) {
        return Nan::ThrowError(mdb_strerror(rc));
    }

    return;
}


NAN_METHOD(TxnWrap::putBinary) {
  Nan::HandleScope scope;

  TxnWrap *tw = Nan::ObjectWrap::Unwrap<TxnWrap>(info.This());
  DbiWrap *dw = Nan::ObjectWrap::Unwrap<DbiWrap>(info[0]->ToObject());

  if (!tw->txn) {
      return Nan::ThrowError("The transaction is already closed.");
  }

  int flags = 0;
  MDB_val key, data;

  key.mv_size = node::Buffer::Length(info[1]);
  key.mv_data = node::Buffer::Data(info[1]);

  data.mv_size = node::Buffer::Length(info[2]);
  data.mv_data = node::Buffer::Data(info[2]);

  int rc = mdb_put(tw->txn, dw->dbi, &key, &data, flags);

  if (rc != 0) {
      return Nan::ThrowError(mdb_strerror(rc));
  }

  return;
}
