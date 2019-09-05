// 64 bits
/*
0:  48 b8 45 55 46 84 45    movabs rax,0x454584465545
7:  45 00 00
a:  ff e0                   jmp    rax
*/

// 32 bits
/*
0:  b8 01 00 00 00          mov    eax,0x1
5:  ff e0                   jmp    eax
*/

class CDetour
{
public:

    CDetour( Pointer pFunction , Pointer pNewFunction )
        : m_pFunction( nullptr ) , m_pNewFunction( nullptr ) , m_SavedBytes{ 0 } , m_PageSize( 0 ) , m_PageStart( 0 )
    #ifdef MX64
        , m_LongJmp{ 0x48 , 0xB8 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0xFF , 0xE0 }
    #else
        , m_LongJmp{ 0xB8 , 0x00 , 0x00 , 0x00 , 0x00 , 0xFF , 0xE0 }
    #endif
    {
        m_pFunction = pFunction;
        m_pNewFunction = pNewFunction;
        m_SavedBytes.resize( m_LongJmp.size() );

        m_PageSize = ( unsigned long ) sysconf( _SC_PAGE_SIZE );

        m_PageStart = reinterpret_cast< uintptr_t >( m_pFunction ) & -m_PageSize;

        memcpy( m_SavedBytes.data() , m_pFunction , m_SavedBytes.size() );
    #ifdef MX64
        memcpy( reinterpret_cast< Pointer >( reinterpret_cast< uintptr_t >( m_LongJmp.data() ) + 2 ) , &m_pNewFunction , sizeof( Pointer ) );
    #else
        memcpy( reinterpret_cast< Pointer >( reinterpret_cast< uintptr_t >( m_LongJmp.data() ) + 1 ) , &m_pNewFunction , sizeof( Pointer ) );
    #endif
    }

    FORCEINLINE auto IsValid() -> bool
    {
        return IsPointer( m_pFunction ) && IsPointer( m_pNewFunction );
    }

    auto Detour() -> bool
    {
        if ( !IsValid() )
            return false;

        if ( mprotect( reinterpret_cast< Pointer >( m_PageStart ) , m_PageSize , PROT_READ | PROT_WRITE | PROT_EXEC ) != -1 )
        {
            memcpy( m_pFunction , m_LongJmp.data() , m_LongJmp.size() );

            // We assume that it was read + exec.
            return mprotect( reinterpret_cast< Pointer >( m_PageStart ) , m_PageSize , PROT_READ | PROT_EXEC ) != -1;
        }

        return false;
    }

    auto Reset() -> bool
    {
        if ( !IsValid() )
            return false;

        if ( mprotect( reinterpret_cast< Pointer >( m_PageStart ) , m_PageSize , PROT_READ | PROT_WRITE | PROT_EXEC ) != -1 )
        {
            memcpy( m_pFunction , m_SavedBytes.data() , m_SavedBytes.size() );

            // We assume that it was read + exec.
            return mprotect( reinterpret_cast< Pointer >( m_PageStart ) , m_PageSize , PROT_READ | PROT_EXEC ) != -1;
        }

        return false;
    }

    FORCEINLINE auto GetFunction() -> Pointer
    {
        return m_pFunction;
    }

    FORCEINLINE auto GetNewFunction() -> Pointer
    {
        return m_pNewFunction;
    }

    template< typename RetType = void , typename ... vArgs >
    constexpr FORCEINLINE auto _CallFunction( vArgs ... pArgs ) -> RetType
    {
        return reinterpret_cast< RetType(*)( vArgs ... ) >( m_pFunction )( pArgs ... );
    }

    template< typename RetType = void , typename ... vArgs >
    FORCEINLINE auto CallFunction( vArgs ... pArgs ) -> RetType
    {
        if ( Reset() )
        {
            _CallFunction<RetType , vArgs...>( pArgs... );
            Detour();
        }
    }

    ~CDetour()
    {
        Reset();
        m_LongJmp.clear();
        m_SavedBytes.clear();
        m_pFunction = nullptr;
        m_pNewFunction = nullptr;
    }

private:
    Pointer m_pFunction , m_pNewFunction;
    std::vector<BYTE> m_SavedBytes;
    unsigned long m_PageSize;
    uintptr_t m_PageStart;
    std::vector<BYTE> m_LongJmp;

};

typedef CDetour* pCDetour;
