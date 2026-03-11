#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <climits>

using namespace std;

struct TrieNode
{
    bool isEnd = false;                    // indicates end of prefix
    std::string nextHop;                   // or associated data
    std::unique_ptr<TrieNode> children[2]; // 0 or 1 bit
};

class LPM
{
private:
    TrieNode root;

    // Convert IP string "192.168.1.1" to 32-bit integer
    uint32_t ipToInt(const std::string &ip)
    {
        uint32_t res = 0;
        size_t start = 0;
        for (int i = 0; i < 4; i++)
        {
            size_t pos = ip.find('.', start);
            int octet = std::stoi(ip.substr(start, pos - start));
            res = (res << 8) | (octet & 0xFF);
            start = pos + 1;
        }
        return res;
    }

public:
    // Insert prefix in format "192.168.0.0/16"
    void insert(const std::string &prefix, const std::string &nextHop)
    {
        size_t slash = prefix.find('/');
        std::string ipStr = prefix.substr(0, slash);
        int mask = std::stoi(prefix.substr(slash + 1));
        uint32_t ip = ipToInt(ipStr);

        TrieNode *node = &root;
        for (int i = 31; i >= 32 - mask; i--)
        {
            int bit = (ip >> i) & 1;
            if (!node->children[bit])
                node->children[bit] = std::make_unique<TrieNode>();
            node = node->children[bit].get();
        }
        node->isEnd = true;
        node->nextHop = nextHop;
    }

    // Lookup IP address and return longest matching prefix's next hop
    std::string lookup(const std::string &ipStr)
    {
        uint32_t ip = ipToInt(ipStr);
        TrieNode *node = &root;
        std::string result = ""; // default: no match

        for (int i = 31; i >= 0; i--)
        {
            int bit = (ip >> i) & 1;
            if (!node->children[bit])
                break;
            node = node->children[bit].get();
            if (node->isEnd)
                result = node->nextHop;
        }
        return result;
    }
};

class LRUCache
{
private:
    size_t capacity;

    // Doubly-linked list to store keys in order of use
    std::list<int> keys; // front = most recent, back = least recent

    // Map key → pair(value, iterator to list node)
    std::unordered_map<int, std::pair<int, std::list<int>::iterator>> cache;

public:
    LRUCache(size_t cap) : capacity(cap) {}

    int get(int key)
    {
        auto it = cache.find(key);
        if (it == cache.end())
            return -1; // Key not found

        // Move accessed key to front
        keys.erase(it->second.second);
        keys.push_front(key);
        it->second.second = keys.begin();

        return it->second.first;
    }

    void put(int key, int value)
    {
        auto it = cache.find(key);

        if (it != cache.end())
        {
            // Key exists → update value and move to front
            it->second.first = value;
            keys.erase(it->second.second);
            keys.push_front(key);
            it->second.second = keys.begin();
        }
        else
        {
            // New key
            if (cache.size() >= capacity)
            {
                // Evict least recently used
                int lruKey = keys.back();
                keys.pop_back();
                cache.erase(lruKey);
            }

            // Insert new key
            keys.push_front(key);
            cache[key] = {value, keys.begin()};
        }
    }
};

class RateLimiter
{
private:
    struct Bucket
    {
        size_t tokens;
        chrono::steady_clock::time_point lastUpdate;
    };

    size_t capacity;
    size_t refillRate;
    unordered_map<int, Bucket> clients;

public:
    RateLimiter(size_t capacity, size_t refillRate)
        : capacity(capacity), refillRate(refillRate) {}

    bool isAllowed(int clientId, size_t amount)
    {
        auto now = chrono::steady_clock::now();

        auto &bucket = clients[clientId];
        if (bucket.lastUpdate.time_since_epoch().count() == 0)
        {
            bucket.tokens = capacity;
            bucket.lastUpdate = now;
        }

        auto elapsed = chrono::duration_cast<chrono::seconds>(now - bucket.lastUpdate).count();

        size_t refill = elapsed * refillRate;
        bucket.tokens = min(capacity, bucket.tokens + refill);

        bucket.lastUpdate = now;

        if (bucket.tokens < amount)
            return false;

        bucket.tokens -= amount;
        return true;
    }
};

class CircularBuffer
{
private:
    size_t capacity;
    vector<int> buffer = {0};
    int head = 0;
    int tail = 0;
    int size = 0;

public:
    CircularBuffer(size_t capacity) : capacity(capacity)
    {
        buffer.reserve(capacity);
    }
    bool isFull()
    {
        return size == capacity;
    }
    bool isEmpty()
    {
        return size == 0;
    }
    void push(int value)
    {
        if (isFull())
        {
            cout << "Buffer is full" << endl;
            return;
        }

        buffer[head] = value;
        head = (head + 1) % capacity;
        size++;
    }
    void pop()
    {
        if (isEmpty())
        {
            cout << "Buffer is empty" << endl;
            return;
        }

        buffer[tail] = 0;
        tail = (tail + 1) % capacity;
        size--;

        return;
    }
    void print()
    {
        for (int i = 0; i < capacity; i++)
            cout << "Buffer[" << i << "] = " << buffer[i] << endl;
    }
    void printHead()
    {
        cout << "head is at : " << head << " : Value is : " << buffer[head] << endl;
    }
    void printTail()
    {
        cout << "tail is at : " << tail << " : Value is : " << buffer[tail] << endl;
    }
};

template <class T>
class TreeNode
{
private:
    T value;
    TreeNode<T> *leftNode = nullptr;
    TreeNode<T> *rightNode = nullptr;

public:
    TreeNode(T value)
        : value(value)
    {
    }

    T getValue() { return value; }
    TreeNode<T> *getLeft() { return leftNode; }
    TreeNode<T> *getRight() { return rightNode; }

    void setLeft(TreeNode<T> *node)
    {
        leftNode = node;
    }

    void setRight(TreeNode<T> *node)
    {
        rightNode = node;
    }
};

class BST
{
private:
    TreeNode<int> *root = nullptr;

    void addNodeInternal(TreeNode<int> *parent, TreeNode<int> *node)
    {
        if (node->getValue() < parent->getValue())
        {
            if (!parent->getLeft())
            {
                parent->setLeft(node);
                return;
            }
            else
            {
                addNodeInternal(parent->getLeft(), node);
            }
        }
        else if (node->getValue() > parent->getValue())
        {
            if (!parent->getRight())
            {
                parent->setRight(node);
                return;
            }
            else
            {
                addNodeInternal(parent->getRight(), node);
            }
        }
        else
        {
            cout << "Not added" << endl;
        }
    }

public:
    BST(TreeNode<int> *root)
        : root(root)
    {
    }
    TreeNode<int> *getRoot()
    {
        return root;
    }
    void addNode(TreeNode<int> *node)
    {
        if (root == nullptr)
        {
            root = node;
            return;
        }

        return addNodeInternal(root, node);
    }
};

template <class T>
void depthFirstTraversal(TreeNode<T> *root)
{
    if (!root)
        return;

    stack<TreeNode<T> *> fifo;
    fifo.push(root);
    while (!fifo.empty())
    {
        auto current = fifo.top();
        fifo.pop();

        if (current)
            cout << "Value of current : " << current->getValue() << endl;

        if (current->getRight())
            fifo.push(current->getRight());
        if (current->getLeft())
            fifo.push(current->getLeft());
    }
}

void pathsSum(TreeNode<int> *root)
{
    if (!root)
        return;

    vector<int> pathSums;
    stack<pair<TreeNode<int> *, int>> s;
    s.push({root, root->getValue()});

    while (!s.empty())
    {
        pair<TreeNode<int> *, int> currentPair = s.top();
        TreeNode<int> *node = currentPair.first;
        int currentSum = currentPair.second;
        s.pop();

        if (!node->getLeft() && !node->getRight())
        {
            pathSums.push_back(currentSum);
        }

        if (node->getRight())
            s.push({node->getRight(), currentSum + node->getRight()->getValue()});

        if (node->getLeft())
            s.push({node->getLeft(), currentSum + node->getLeft()->getValue()});
    }

    for (size_t i = 0; i < pathSums.size(); i++)
    {
        cout << "Path " << i + 1 << " Sum : " << pathSums[i] << endl;
    }
}

void pathString(TreeNode<string> *root)
{
    if (root == nullptr)
        return;

    pair<TreeNode<string> *, string> path;
    vector<string> paths;
    stack<pair<TreeNode<string> *, string>> s;
    s.push({root, root->getValue()});
    while (!s.empty())
    {
        pair<TreeNode<string> *, string> node = s.top();
        s.pop();
        if (!node.first->getLeft() && !node.first->getRight())
            paths.push_back(node.second);

        if (node.first->getRight())
            s.push({node.first->getRight(), node.second + node.first->getRight()->getValue()});

        if (node.first->getLeft())
            s.push({node.first->getLeft(), node.second + node.first->getLeft()->getValue()});
    }

    for (size_t i = 0; i < paths.size(); i++)
    {
        cout << "Paths " << i + 1 << " path : " << paths[i] << endl;
    }
}

void printBits(uint8_t number)
{
    cout << "Bits : ";
    for (int i = 0; i < 8; i++)
    {
        auto bit = (number >> (8 - 1 - i)) & 1;
        cout << bit;
    }
    cout << endl;
}

void flipBits(uint8_t number)
{
    uint8_t flipped = 0;
    cout << "Number ";
    printBits(number);
    for (int i = 0; i < sizeof(number) * 8; i++)
    {
        auto bit = ~((number >> i) & 1) & 1;
        flipped |= (bit << (8 - 1 - i));
    }
    cout << "Flipped ";
    printBits(flipped);
}

class Solution
{
public:
    int reverse(int x)
    {
        vector<int> units;
        int sign = x > 0 ? 1 : -1;
        int result = 0;
        while (1)
        {
            auto unit = x % 10;
            x /= 10;
            units.push_back(unit);
            if (x <= 0)
                break;
        }

        for (int i = 0; i < units.size(); i++)
            result += units[i] * pow(10, units.size() - 1 - i);

        return result * sign;
    }
};

class Solution
{
public:
    int reverse(int x)
    {
        int result = 0;

        while (x != 0)
        {
            int digit = x % 10;
            x /= 10;

            // Check overflow BEFORE multiplying
            if (result > INT_MAX / 10 || 
               (result == INT_MAX / 10 && digit > 7))
                return 0;

            if (result < INT_MIN / 10 || 
               (result == INT_MIN / 10 && digit < -8))
                return 0;

            result = result * 10 + digit;
        }

        return result;
    }
};

int main()
{
    // Solution s;
    // s.reverse(123);
    // LPM lpm;
    // lpm.insert("192.168.0.0/16", "NextHopA");
    // lpm.insert("192.168.1.0/24", "NextHopB");
    // lpm.insert("10.0.0.0/8", "NextHopC");

    // std::vector<std::string> ips = {"192.168.1.5", "192.168.2.10", "10.1.2.3", "8.8.8.8"};

    // for (auto ip : ips)
    // {
    //     std::cout << ip << " -> " << lpm.lookup(ip) << "\n";
    // }

    // BST bst(new TreeNode<int>(5));
    // bst.addNode(new TreeNode<int>(3));
    // bst.addNode(new TreeNode<int>(7));
    // bst.addNode(new TreeNode<int>(2));
    // bst.addNode(new TreeNode<int>(4));
    // bst.addNode(new TreeNode<int>(6));
    // bst.addNode(new TreeNode<int>(8));

    // depthFirstTraversal<int>(bst.getRoot());
    // pathsSum(bst.getRoot());

    // flipBits(12);

    // TreeNode<int> a(1);
    // TreeNode<int> b(2);
    // TreeNode<int> c(3);
    // TreeNode<int> d(4);
    // TreeNode<int> e(5);
    // TreeNode<int> f(6);

    // TreeNode<string> a("1");
    // TreeNode<string> b("2");
    // TreeNode<string> c("3");
    // TreeNode<string> d("4");
    // TreeNode<string> e("5");
    // TreeNode<string> f("6");

    // a.setLeft(&b);
    // a.setRight(&c);
    // b.setLeft(&d);
    // b.setRight(&e);
    // c.setRight(&f);

    // depthFirstTraversal<int>(&a);
    // pathString(&a);

    // auto limiter = RateLimiter(10, 1);
    // cout << "Allowed : " << limiter.isAllowed(1, 10) << endl;
    // cout << "Allowed : " << limiter.isAllowed(1, 1) << endl;

    // auto buffer = CircularBuffer(3);
    // buffer.push(1);
    // buffer.print();
    // buffer.push(2);
    // buffer.print();
    // buffer.push(3);
    // buffer.print();
    // buffer.push(4);
    // buffer.pop();
    // buffer.push(4);
    // buffer.print();
    // buffer.pop();
    // buffer.pop();
    // buffer.pop();
    // buffer.print();
    // buffer.pop();
    // buffer.push(5);
    // buffer.print();
    // buffer.printHead();
    // buffer.printTail();
    // buffer.pop();
    // buffer.printTail();

    return 0;
}
