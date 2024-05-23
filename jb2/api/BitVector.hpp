/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef JB2_BITVECTOR_INCL
#define JB2_BITVECTOR_INCL

#include <cassert>
#include <cstring>
#include <cstdint>
#include "Allocator.hpp"
#include "IDs.hpp"
#include "Iterator.hpp"

namespace OMR {
namespace JitBuilder {

class BitVector {

    typedef uint64_t ChangeID;

    typedef uint64_t WordType;
    static const WordType WORD_SHIFT=6;
    static const WordType WORD_MASK=((((WordType)1) << WORD_SHIFT) - ((WordType)1));

    #define WORD(i)        ((i) >> WORD_SHIFT)
    #define BIT(i)         ((i) & WORD_MASK)
    #define BITWORD(i)     (((WordType)1) << BIT(i))
    #define TOTALWORDS(i)  (WORD(i) + (((BIT(i)) == 0) ? 0 : 1))

public:

    class ForwardIterator : public OMR::JitBuilder::Iterator<bool> {
    public:
        ForwardIterator() // empty iterator
            : Iterator<bool>(NULL)
            , _vector(NULL)
            , _changeAtCreation(0)
            , _bitIndex(0)
            , _detectChanges(false) {
        }

        ForwardIterator(Allocator *a, BitVector * vector, bool detectChanges)
            : Iterator<bool>(a)
            , _vector(vector)
            , _changeAtCreation(vector->_changeID)
            , _bitIndex(0)
            , _detectChanges(detectChanges) {
            reset();
        }

        // rule of 3
        ForwardIterator(const ForwardIterator & other)
            : Iterator<bool>(other)
            , _vector(other._vector)
            , _changeAtCreation(other._changeAtCreation)
            , _bitIndex(other._bitIndex)
            , _detectChanges(other._detectChanges) {

        }
        virtual ~ForwardIterator() { }
        ForwardIterator operator=(const ForwardIterator & other) {
            _vector = other._vector;
            _changeAtCreation = other._changeAtCreation;
            _bitIndex = other._bitIndex;
            _detectChanges = other._detectChanges;
            return *this;
        }

        // rule of 5
        ForwardIterator(ForwardIterator && other)
            : Iterator<bool>(other)
            , _vector(other._vector)
            , _changeAtCreation(other._changeAtCreation)
            , _bitIndex(other._bitIndex)
            , _detectChanges(other._detectChanges) {
        }
        ForwardIterator & operator=(const ForwardIterator && other) {
            _vector = other._vector;
            _changeAtCreation = other._changeAtCreation;
            _bitIndex = other._bitIndex;
            _detectChanges = other._detectChanges;
            return *this;
        }

        void reset() {
            if (!_vector) return;
            checkForChange();
            _bitIndex = 0;
            findSetBit();
        }
        bool hasItem() {
            if (!_vector) return false;
            checkForChange();
            return _bitIndex < _vector->_length;
        }
        void operator++(int) {
            checkForChange();
            _bitIndex++;
            findSetBit();
        }
        BitIndex item() const {
            checkForChange();
            assert(_bitIndex < _vector->_length);
            return _bitIndex;
        }

    protected:
        void checkForChange() const {
            if (_detectChanges)
                assert(_vector != NULL && _changeAtCreation == _vector->_changeID);
        }
        void findSetBit() {
            if (_vector->_words == NULL)
                return;
            if (!_vector->directGetBit(_bitIndex)) {
                while (_bitIndex <= _vector->_length) {
                    _bitIndex++;
                    if (_vector->directGetBit(_bitIndex))
                        break;
                }
            }
        }

        BitVector *_vector;
        ChangeID _changeAtCreation;
        BitIndex _bitIndex;
        bool _detectChanges;
    };

public:
    BitVector(Allocator *a)
        : _mem(a)
        , _changeID(0)
        , _length(0)
        , _words(NULL)
        , _ownWords(false) {
    }

    // rule of 3
    BitVector(const BitVector & other)
        : _mem(other._mem)
        , _changeID(other._changeID)
        , _length(other._length)
        , _words(copy(other._words, other._length)) 
        , _ownWords(true) {
    }
    virtual ~BitVector() {
        if (_ownWords && _words != NULL)
            _mem->deallocate(_words);
    }
    void operator=(const BitVector & other) {
        _mem = other._mem;
        _changeID = other._changeID;
        _length = other._length;
        _words = copy(other._words, other._length);
        _ownWords = true;
    }

    // rule of 5
    BitVector(BitVector && other)
        : _mem(other._mem)
        , _changeID(other._changeID)
        , _length(other._length)
        , _words(other._words)
        , _ownWords(true) {

        other._ownWords = false;
    }
    void operator=(BitVector && other) {
        _mem = other._mem;
        _changeID = other._changeID;
        _length = other._length;
        _words = other._words;
        _ownWords = true;
        other._ownWords = false;
    }

    BitVector(Allocator *a, BitIndex sizeHint)
        : _mem(a)
        , _changeID(0)
        , _length(0)
        , _words(NULL)
        , _ownWords(false) {
        grow(sizeHint);
    }

    BitVector(Allocator *a, BitIndex sizeHint, BitIndex one)
        : _mem(a)
        , _changeID(0)
        , _length(0)
        , _words(NULL)
        , _ownWords(false) {
        grow(sizeHint);
        setBit(one);
    }

    BitVector(Allocator *a, BitIndex sizeHint, BitIndex one, BitIndex two)
        : _mem(a)
        , _changeID(0)
        , _length(0)
        , _words(NULL)
        , _ownWords(false) {
        grow(sizeHint);
        setBit(one);
        setBit(two);
    }

    BitVector(Allocator *a, BitIndex sizeHint, BitIndex one, BitIndex two, BitIndex three)
        : _mem(a)
        , _changeID(0)
        , _length(0)
        , _words(NULL)
        , _ownWords(false) {
        grow(sizeHint);
        setBit(one);
        setBit(two);
        setBit(three);
    }

    BitIndex length() const { return _length; }

    bool getBit(BitIndex index) const {
        if (index >= _length || _length == 0)
            return false;
        return directGetBit(index);
    }
    bool operator[] (BitIndex index) const {
        if (index >= _length || _length == 0)
            return false;
        return getBit(index);
    }
    void setBit(BitIndex index, bool v=true) {
        if (index >= _length)
            grow(index);
        _changeID++; // assume A[i] = x but imperfect accounting unless we wrap the T
        if (v)
            _words[WORD(index)] |= BITWORD(index);
        else
            _words[WORD(index)] &= ~(BITWORD(index));
    }
    void clear() {
        for (WordType w=0;w < TOTALWORDS(_length); w++)
            _words[w] = 0;
    }
    void erase() {
        if (_ownWords && _words != NULL)
            _mem->deallocate(_words);
        _length = 0;
        _ownWords = false;
        _changeID++;
    }

    ForwardIterator iterator(bool detectChanges=true) {
        return ForwardIterator(_mem, this, detectChanges);
    }
    ForwardIterator fwdIterator(bool detectChanges=true) {
        return ForwardIterator(_mem, this, detectChanges);
    }
    #if 0
    Array<T>::BackwardIterator revIterator(bool detectChanges=true) const {
        return List<T>::BackwardIterator(this, detectChanges);
    }
    #endif

protected:
    friend class ForwardIterator;

    bool directGetBit(BitIndex index) const {
        return (_words[WORD(index)] & (BITWORD(index))) != 0;
    }
    WordType *copy(WordType *source, BitIndex length) {
        WordType *mem = _mem->allocate<WordType>(TOTALWORDS(length));
        size_t words = TOTALWORDS(length);
        memcpy(mem, source, words);
        return mem;
    }

    void grow(BitIndex indexNeeded) {
        if (indexNeeded < _length)
            return;

        BitIndex newLength = indexNeeded+1;
        bool needDeallocate = _length > 0;

        WordType *newWords = _mem->allocate<WordType>(TOTALWORDS(newLength));
        assert(newWords != NULL);

        WordType *zeroStart = newWords;
        size_t oldWords = TOTALWORDS(_length);
        if (oldWords > 0) {
            memcpy(newWords, _words, sizeof(WordType) * oldWords);
            zeroStart += oldWords;
        }
        size_t zeroWords = TOTALWORDS(newLength) - oldWords;
        memset(zeroStart, 0, sizeof(WordType) * zeroWords);
        if (needDeallocate)
            _mem->deallocate(_words);

        _ownWords = true;
        _words = newWords;
        _length = newLength;
    }

    Allocator *_mem;
    ChangeID _changeID;
    BitIndex _length;
    WordType *_words;
    bool _ownWords;
};

} // namespace JitBuilder
} // namespace OMR

#undef WORD
#undef BIT
#undef BITWORD
#undef TOTALWORDS

#endif // defined(JB2_BITVECTOR_INCL)
