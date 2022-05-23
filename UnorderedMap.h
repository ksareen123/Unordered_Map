#include <cstddef>    // size_t
#include <functional> // std::hash
#include <utility>    // std::pair
#include <iostream>

#include "primes.h"

using namespace std;

template <typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>>
class UnorderedMap {
    public:

    using key_type = Key;
    using mapped_type = T;
    using hasher = Hash;
    using key_equal = Pred;
    using value_type = std::pair<const key_type, mapped_type>;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    private:

    struct HashNode {
        HashNode *next;
        value_type val;

        HashNode(HashNode *next = nullptr) : next{next} {}
        HashNode(const value_type & val, HashNode * next = nullptr) : next { next }, val { val } { }
        HashNode(value_type && val, HashNode * next = nullptr) : next { next }, val { std::move(val) } { }
    };

    HashNode **_buckets;
    size_type _size;
    size_type _bucket_count;

    HashNode _head;

    Hash _hash;
    key_equal _equal;

    static size_type _range_hash(size_type hash_code, size_type bucket_count) {
        return hash_code % bucket_count;
    }

    public:
    void traverse() {
        for (size_t n = 0; n < _bucket_count; n++) {
            local_iterator node = begin(n);
            cout << "Bucket " << n << ": " << endl;
            while (node != end(n)) {
                cout << node->first << ", ";
                ++node;
            }
            cout << endl;
            cout << "reached end of bucket " << n << endl;
        }
    }
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const key_type, mapped_type>;
        using difference_type = ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

    private:
        friend class UnorderedMap<Key, T, Hash, key_equal>;
        using HashNode = typename UnorderedMap<Key, T, Hash, key_equal>::HashNode;

        HashNode * _node;

        explicit iterator(HashNode *ptr) noexcept { _node = ptr; }

    public:
        iterator() { _node = nullptr; };
        iterator(const iterator &) = default;
        iterator(iterator &&) = default;
        ~iterator() = default;
        iterator &operator=(const iterator &) = default;
        iterator &operator=(iterator &&) = default;
        reference operator*() const { 
            return _node->val; 
        }
        pointer operator->() const { 
            return &(_node->val); 
        }
        iterator &operator++() {
            _node = _node->next;
            return *this;
        }
        iterator operator++(int) {
            iterator old(_node);
            _node = _node->next;
            return old;
        }
        bool operator==(const iterator &other) const noexcept {
            return (_node == other._node);
        }
        bool operator!=(const iterator &other) const noexcept {
            return !(_node == other._node);
        }
    };

    class local_iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::pair<const key_type, mapped_type>;
            using difference_type = ptrdiff_t;
            using pointer = value_type *;
            using reference = value_type &;

        private:
            friend class UnorderedMap<Key, T, Hash, key_equal>;
            using HashNode = typename UnorderedMap<Key, T, Hash, key_equal>::HashNode;

            UnorderedMap * _map;
            HashNode * _node;
            size_type _bucket;

            explicit local_iterator(UnorderedMap * map, HashNode *ptr, size_type bucket) noexcept { 
                _map = map;
                _node = ptr;
                _bucket = bucket;
            }

        public:
            local_iterator() { 
                _map = nullptr;
                _node = nullptr;
                _bucket = 0;
            };
            local_iterator(const local_iterator &) = default;
            local_iterator(local_iterator &&) = default;
            ~local_iterator() = default;
            local_iterator &operator=(const local_iterator &) = default;
            local_iterator &operator=(local_iterator &&) = default;
            reference operator*() const { return _node->val; }
            pointer operator->() const { return &(_node->val); }
            local_iterator & operator++() {
                if (_node == nullptr) {
                    return *this;
                }
                // cout << "in bucket " << _bucket << endl;
                if (_node->next == nullptr || _map->_bucket(_node->next->val.first) != _bucket) {
                    // cout << "end of bucket" << endl;
                    _node = nullptr;
                    return *this;
                }
                else {
                    // cout << "normal case" <<endl;
                    _node = _node->next; 
                    return *this;
                }
            }
            local_iterator operator++(int) {
                if (_node == nullptr) {
                    return *this;
                }
                if (_node->next == nullptr || _map->_bucket(_node->next->val.first) != _bucket) {
                    // cout << "end of bucket" << endl;
                    local_iterator old(_map, _node, _bucket);
                    _node = nullptr;
                    return old;
                }
                else {
                    // cout << "normal case" << endl;
                    local_iterator old(_map, _node, _bucket);
                    _node = _node->next;
                    return old;
                }
            }
            bool operator==(const local_iterator &other) const noexcept {
                return (_node == other._node);
            }
            bool operator!=(const local_iterator &other) const noexcept {
                if (_node != other._node) {
                    return true;
                }
                return false;
            }
    };

private:
    size_type _bucket(size_t code) const {
        return _range_hash(code, _bucket_count);
    }
    size_type _bucket(const Key &key) const { 
        return _bucket(_hash(key));
     }

    void _insert_before(size_type bucket, HashNode *node) {
        if (_buckets[bucket]) { // case if non empty
            node->next = _buckets[bucket]->next; 
            _buckets[bucket]->next = node; 
        }
        else {
            if (!_head.next) { // list is empty
                _buckets[bucket] = &_head;
                _head.next = node;
                return;
            }
            _buckets[bucket] = &_head; // list contains elements, bucket empty
            size_t next_bucket = _bucket(_head.next->val.first);
            _buckets[next_bucket] = node;
            node->next = _head.next;
            _head.next = node;
            return;
        }
    }

    HashNode*& _bucket_begin(size_type bucket) {
        if (_buckets[_bucket(bucket)] == nullptr) {
            return _buckets[_bucket(bucket)];
        }
        return _buckets[_bucket(bucket)]->next;
    }

    HashNode* _find_prev(size_type code, size_type bucket, const Key & key) {
        if (!_buckets[bucket]) { //check if empty bucket
            return nullptr;
        }
        HashNode* curr = _buckets[bucket]->next; //create ptrs for linked list search
        HashNode* prev = _buckets[bucket];

        while (curr != nullptr) { //loop through bucket, compare curr until found
            if (_equal(curr->val.first, key)) {
                return prev; // return the previous node
            }
            prev = curr;
            curr = curr->next;
        }

        return nullptr;
    }

    HashNode* _find_prev(const Key & key) {
        return _find_prev(_hash(key), _bucket(key), key); // run method with 3 args off this
    }

    void _erase_after(HashNode * prev) {
        if (!prev) {
            cout << "did not find value" << endl;
            return;
        }
        HashNode* curr = prev->next;
        HashNode* aft = curr->next;
        size_t curr_bucket = _bucket(curr->val.first);
        size_t prev_bucket = _bucket(prev->val.first);
        if (aft) {
            size_t aft_bucket = _bucket(aft->val.first);
            if (prev != &_head && prev_bucket == aft_bucket) { // normal case, middle of bucket
                cout << "normal case" << endl;
              prev->next = aft;
              delete curr;  
              _size--;
              return;
            }
            else {
                cout << "end of bucket case" << endl;
                if (curr_bucket != aft_bucket) { // end of bucket
                    _buckets[aft_bucket] = prev; // rewire bucket
                    prev->next = aft;
                    delete curr;
                    _size--;
                    if(prev == &_head || prev_bucket != curr_bucket) {
                        _buckets[curr_bucket] = nullptr;
                    }
                    return;
                }
                prev->next = aft;
                delete curr;  
                _size--;
                return;
            }
        }
        cout << "end of list case" << endl;
        prev->next = nullptr;
        delete curr;
        _size--;
        
        if(prev == &_head || prev_bucket != curr_bucket) {
            _buckets[curr_bucket] = nullptr;
        }
        return;
        _size--;

    }

public:
    explicit UnorderedMap(size_type bucket_count, const Hash & hash = Hash { },
                const key_equal & equal = key_equal { }) {
        
        _bucket_count = next_greater_prime(bucket_count);
        _size = 0;
        _head.next = nullptr;
        _hash = hash;
        _equal = equal;
        _buckets = new HashNode*[_bucket_count]{};
    }

    ~UnorderedMap() {
        clear();
    }

    UnorderedMap(const UnorderedMap & other) {
        // initialize members
        _hash = other._hash;
        _equal = other._equal;
        
        _bucket_count = other.bucket_count();
        _buckets = new HashNode*[_bucket_count]{};

        HashNode* curr = other._head.next; // parse through linked list and insert
        while (curr != nullptr) {
            insert(curr->val);
            curr = curr->next;
        }
    }

    UnorderedMap(UnorderedMap && other) {
        // if (other.empty()) {
        //     _bucket_count = other._bucket_count;
        //     _size = 0;
        //     _head.next = nullptr;
        //     _hash = other._hash;
        //     _equal = other._equal;
        //     _buckets = new HashNode*[_bucket_count]{};
        // }
        _head.next = other._head.next;
        _hash = other._hash;
        _equal = other._equal;
        _bucket_count = other.bucket_count();
        _size = other._size;
        _head.next = other._head.next;
        _buckets = other._buckets;
        _buckets[_bucket(_head.next->val.first)] = &_head;
        other._head.next = nullptr;
        other._buckets = new HashNode*[other.bucket_count()]{};
    }

    UnorderedMap & operator=(const UnorderedMap & other) {
        if (other.empty()) {
            return *this;
        }
        clear();

        _hash = other._hash;
        _equal = other._equal;
        
        _bucket_count = other.bucket_count();
        delete _buckets;
        _buckets = new HashNode*[_bucket_count]{};

        HashNode* curr = other._head.next; // parse through linked list and insert
        while (curr != nullptr) {
            insert(curr->val);
            curr = curr->next;
        }
        return *this;
    }

    UnorderedMap & operator=(UnorderedMap && other) {
        // cout << "here" << endl;
        if (this == &other) {
            return *this;
        }
        clear();
        delete _buckets;
        _head.next = other._head.next;
        _hash = other._hash;
        _equal = other._equal;
        _bucket_count = other.bucket_count();
        _size = std::move(other._size);
        other._size = 0;
        _head.next = other._head.next;
        _buckets = other._buckets;
        _buckets[_bucket(_head.next->val.first)] = &_head;
        other._head.next = nullptr;
        other._buckets = new HashNode*[other.bucket_count()]{};
        cout << "moved operation, new size " << size() << endl;
        return * this;
    }

    void clear() noexcept {
        if (empty()) { // case if empty
            return;
        }
        // for (size_t i = 0; i < _bucket_count; i++) { // delete each element of array
        //     delete _buckets[i];
        // }
        // delete[] _buckets; // delete buckets pointer
        HashNode * currnode = _head.next; // delete ll using 2 pointers
        while (currnode != nullptr) {
            _head.next = currnode->next;
            delete currnode;
            currnode = _head.next;
        }
        _head.next = nullptr;
        _size = 0; // reset size
    }

    size_type size() const noexcept { return _size; }

    bool empty() const noexcept { return (_size == 0); }

    size_type bucket_count() const noexcept { return _bucket_count; }

    iterator begin() { 
        return iterator(_head.next); 
    }

    iterator end() { 
        return iterator(nullptr); 
    }

    local_iterator begin(size_type n) { 
        HashNode* beg = _buckets[_bucket(n)];
        if (beg != nullptr) {
            beg = beg->next;
        }
        return local_iterator(this, beg, n);
    }

    local_iterator end(size_type n) { 
        return local_iterator(this, nullptr, n); 
    }

    size_type bucket_size(size_type n) { return std::distance(begin(n), end(n)); }

    float load_factor() const { return static_cast<double>(size()) / bucket_count(); }

    size_type bucket(const Key & key) const { return _bucket(key); }

    std::pair<iterator, bool> insert(value_type && value) {
        bool pass = false;
        HashNode * prev = _find_prev(value.first);
        if (prev) {
            return std::make_pair(iterator(prev->next), pass); // failed insertion
        }

        pass = true; // successful insertion
        _size++;
        HashNode * mynode = new HashNode(std::move(value), nullptr);
        _insert_before(_bucket(value.first), mynode);
        return std::make_pair(iterator(mynode), pass);
    }

    std::pair<iterator, bool> insert(const value_type & value) {
        bool pass = false;
        HashNode * prev = _find_prev(value.first);
        if (prev) {
            return std::make_pair(iterator(prev->next), pass); // failed insertion
        }

        pass = true; // successful insertion
        _size++;
        HashNode * mynode = new HashNode(value, nullptr);
        _insert_before(_bucket(value.first), mynode);
        return std::make_pair(iterator(mynode), pass);
    }

    iterator find(const Key & key) {
        HashNode * prev = _find_prev(key);
        if (prev != nullptr) {
            return iterator(prev->next);
        }
        else {
            return iterator(prev);
        }
    }

    T& operator[](const Key & key) {
        HashNode* prev = _find_prev(key);
        if (!prev) {
            std::pair<Key,T> mypair;
            mypair.first = key;
            HashNode* mynode = new HashNode(mypair, nullptr);
            _insert_before(_bucket(key), mynode);
            return mynode->val.second;
        }
        return (prev->next->val.second);
    }

    iterator erase(iterator pos) {
        size_t worked = erase(pos->first);
        return ++pos;
    }

    size_type erase(const Key & key) {
        // print_map(*this);
        HashNode* prev = _find_prev(key); 
        cout << "finding previous to key " << key << endl;
        size_t s = size();
        cout << "size: " << s << endl;
        _erase_after(prev);
        cout << "new size: " << size() << endl;
        // print_map(*this);
        if (s == size()) {
            return 0;
        }
        else {
            return 1;
        }
    }

    template<typename KK, typename VV>
    friend void print_map(const UnorderedMap<KK, VV> & map, std::ostream & os);
};

template<typename K, typename V>
void print_map(const UnorderedMap<K, V> & map, std::ostream & os = std::cout) {
    using size_type = typename UnorderedMap<K, V>::size_type;
    using HashNode = typename UnorderedMap<K, V>::HashNode;

    HashNode const * node = map._head.next;
    os << "List: ";
    do {
        if(node) {
            os << "(" << node->val.first << ", " << node->val.second << ") ";
        } else {
            os << "(nullptr)";
            break;
        }

        node = node->next;
    } while(true);
    os << std::endl;

    for(size_type bucket = 0; bucket < map.bucket_count(); bucket++) {
        os << bucket << ": ";

        HashNode const * node = map._buckets[bucket];

        if(!node) {
            os << "(nullptr)";
        } else {
            while((node = node->next) && map.bucket(node->val.first) == bucket) {
                os << "(" << node->val.first << ", " << node->val.second << ") ";
            }
        }
        os << std::endl;
    }
}