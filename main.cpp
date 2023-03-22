#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <cstdlib>

#ifdef _MSC_VER 
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

using namespace std;

static constexpr int POOL_SIZE = 10;

template <class T, size_t N>
struct MyAllocator {
    using value_type = T;

    MyAllocator () noexcept = default;
    
    template <class U> MyAllocator(const MyAllocator <U, N>& a) noexcept {
        allocated_memory = a.allocated_memory;
        used_memory = a.used_memory;
    }

	template <class U>
	struct rebind {
		using other = MyAllocator<U, N>;
	};

	T* allocate( std::size_t n )
    {
        if ( n > N )
            throw std::bad_alloc();

        if ( !allocated_memory ) // first allocation of a large block of data
        {
            used_memory = 0;

            allocated_memory = reinterpret_cast< T* >( std::malloc( N * sizeof( T ) ) );

            if ( !allocated_memory )
                throw std::bad_alloc();

            std::cout << __PRETTY_FUNCTION__ << " allocating block of " << N << " elements @ " << allocated_memory << " used memory " << used_memory << endl;
        }

        if ( used_memory + n <= N )
        {
            T* position = (T*)allocated_memory + used_memory;
            used_memory += n;
            
            std::cout << __PRETTY_FUNCTION__ << " asking for " << n << " elements, used_memory " << used_memory << " returned @ " << position << endl;
            
            return position;
        }
        else
        {
            throw std::bad_alloc();
        }

        return nullptr;
    }

	void deallocate( T* p, std::size_t n )
    {
        std::cout << __PRETTY_FUNCTION__ << " deallocating block of " << n << " elements @ " << p << " used memory " << used_memory << "\n\n";

        if ( p == allocated_memory )
        {
            std::free( p );
            allocated_memory = nullptr;
            used_memory = 0;
            std::cout << "DEALLOC" << std::endl;
        }
        else
        {
            cout << "nothing to deallocate" << endl;
        }
    }

    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;

    void* allocated_memory = nullptr;
	size_t used_memory = 0;
};

template <class T, class U, size_t N>
constexpr bool operator== (const MyAllocator<T, N>& a1, const MyAllocator<U, N>& a2) noexcept
{
    return a1.pool == a2.pool;
}

template <class T, class U, size_t N>
constexpr bool operator!= (const MyAllocator<T, N>& a1, const MyAllocator<U, N>& a2) noexcept
{
    return a1.pool != a2.pool;
}

template <typename T, typename Alloc = std::allocator<T>>
class MyList
{
    struct Node
    {
        Node(const T& new_val) : val(new_val), next(nullptr) {}
        
        T val;
        Node* next = nullptr;
    };
    
    using ReboundAllocType = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
    
public:
    
    MyList() : rebound_allocator(allocator), head(nullptr), tail(nullptr) {};
    
    ~MyList()
    {
        while ( head != nullptr ) {
            pop_front();
        }
    }
    
    void push_back(const T& val)
    {
        Node* new_node = std::allocator_traits<ReboundAllocType>::allocate( rebound_allocator, 1 ) ;
        std::allocator_traits<ReboundAllocType>::construct( rebound_allocator, new_node, val );

        if(empty())
        {
            head = new_node;
            tail = head;
            return;
        }
        
        tail->next = new_node;
        tail = new_node;
    }
    
    void pop_front() {
        if ( head != nullptr ) {
            Node *t = head->next;
            std::allocator_traits<ReboundAllocType>::destroy( rebound_allocator, head );
            std::allocator_traits<ReboundAllocType>::deallocate( rebound_allocator, head, 1 );
            head = t;
        }
    }
    
    bool empty()
    {
        return head == nullptr;
    }
    
    Node* operator[] (const int index)
    {
        if (empty())
            return nullptr;
        Node* p = head;
        for (int i = 0; i < index; ++i) {
            p = p->next;
            if (!p)
                return nullptr;
        }
        return p;
    }
    
    void print()
    {
        if(empty())
            return;
        
        auto* node = head;
        while(node)
        {
            cout << node->val << endl;
            node = node->next;
        }
    }
    
private:
    
    Alloc allocator;
    ReboundAllocType rebound_allocator;
    
    Node* head = nullptr;
    Node* tail = nullptr;
};

int factorial( int n )
{
	return ( n == 1 || n == 0 ) ? 1 : factorial( n - 1 ) * n;
}

int main( [[maybe_unused]] int argc, [[maybe_unused]] char const* argv[] )
{
	{
		cout << "std map with default allocator" << endl;
        {
            map<int, int> factorial_map;
            for ( int i = 0; i < 10; ++i )
                factorial_map[ i ] = factorial( i );

            for ( const auto& value : factorial_map )
                cout << value.first << " " << value.second << endl;

            cout << endl;

            cout << "std map with custom allocator" << endl;

            map<int, int, std::less<int>, MyAllocator<pair<int const, int>, POOL_SIZE>> factorial_map_alloc;
            for ( int i = 0; i < 10; ++i )
                factorial_map_alloc[ i ] = factorial( i );

            for ( const auto& value : factorial_map_alloc )
                cout << value.first << " " << value.second << endl;

            cout << endl;
        }
        
        cout << endl;
        
        cout << "custom list with default allocator" << endl;
        
        {
            MyList<int, std::allocator<int>> list_factorial;
        
            for ( int i = 0; i < 10; ++i )
                list_factorial.push_back(i);
        
            list_factorial.print();

            cout << endl;
            
            cout << "custom list with custom allocator" << endl;
        
            MyList<int, MyAllocator<int, POOL_SIZE>> list_factorial_custom;
        
            for ( int i = 0; i < 10; ++i )
                list_factorial_custom.push_back(i);
        
            list_factorial_custom.print();
        }
        
        cout << endl;
	}

	return 0;
}
