class CHookCall
{
public:

    CHookCall( Pointer pFunction , Pointer pNewFunction )
        : m_FoundFunctionsRVACall( 0 ) , m_pFunction( nullptr ) , m_pNewFunction( nullptr ) , m_pOldRelativeRVA( nullptr )
    {
        m_pFunction = pFunction;
        m_pNewFunction = pNewFunction;
    }

    ~CHookCall()
    {
        UnHook();
        m_FoundFunctionsRVACall.clear();
        m_pFunction = nullptr;
        m_pNewFunction = nullptr;
        m_pOldRelativeRVA = nullptr;
    }

    auto Hook() -> bool
    {
        auto pDosHeader = ( PIMAGE_DOS_HEADER ) FindModule();

        if ( !IsPointer( pDosHeader ) )
            return false;

        auto pNtHeaders = reinterpret_cast< pNTHeaders >( ( reinterpret_cast< uintptr_t >( pDosHeader ) + pDosHeader->e_lfanew ) );

        auto FunctionRVA = reinterpret_cast< uintptr_t >( m_pFunction ) - reinterpret_cast< uintptr_t >( pDosHeader );

        auto ImageSize = pNtHeaders->OptionalHeader.SizeOfImage;

        auto TempModule = Alloc<uintptr_t>( ImageSize );

        // We copy the module so we don't need to change all the protections on it.

        memcpy( ( Pointer ) TempModule , pDosHeader , ImageSize );

        for ( uintptr_t Slot = 0; Slot != ImageSize; Slot++ )
        {
            auto Address = TempModule + Slot;

            BYTE ByteRead = Read<BYTE>( Address );

            if ( ByteRead == 0xE8
                 || ByteRead == 0xE9 )
            {
                // Let's check if it's what we wanted.

                // call 0x...
                // call RelativeAddress ( FunctionToCall - ( FunctionFromBeingCalled + sizeof( Pointer ) ) )
                // or
                // jmp 0x...
                // jmp RelativeAddress ( FunctionToCall - ( FunctionFromBeingCalled + sizeof( Pointer ) ) )

                auto RelativeAddress = Read<uintptr_t>( Address + 1 );

                // + 1 byte for call opcode, and add a size of a pointer
                // because the CPU call our function once it has finished to read the address.

                auto CalledFrom = Address + 1 + sizeof( Pointer );

                auto CalledFromRVA = CalledFrom - TempModule;

                auto CalledFunctionRVA = RelativeAddress + CalledFromRVA;

                // Is That our function ?

                if ( CalledFunctionRVA == FunctionRVA )
                {
                    // Yup.
                    m_FoundFunctionsRVACall.push_back( CalledFromRVA - sizeof( Pointer ) );
                }
            }
        }

        for ( auto && FunctionRVACall : m_FoundFunctionsRVACall )
        {
            auto CallerAddress = reinterpret_cast< uintptr_t >( pDosHeader ) + FunctionRVACall;

            // We want to access it.

            DWORD dwOldFlag;
            if ( VirtualProtect( reinterpret_cast< Pointer >( CallerAddress ) , sizeof( CallerAddress ) , PAGE_EXECUTE_READWRITE , &dwOldFlag ) )
            {
                uintptr_t *pWriteAddress = Write<uintptr_t>( CallerAddress );

                m_pOldRelativeRVA = reinterpret_cast< Pointer >( *pWriteAddress );

                // Add a size of a pointer because the CPU call our function once it has finished to read the address.
                *pWriteAddress = reinterpret_cast< uintptr_t >( m_pNewFunction ) - ( CallerAddress + sizeof( Pointer ) );

                if ( !VirtualProtect( reinterpret_cast< Pointer >( CallerAddress ) , sizeof( CallerAddress ) , dwOldFlag , &dwOldFlag ) )
                {
                    return false;
                }
            }
        }

        FreeS( TempModule );

        PrintConsole( FOREGROUND_GREEN , TEXT( "Found %i references for 0x%p\n" ) , m_FoundFunctionsRVACall.size() , m_pFunction );

        return m_FoundFunctionsRVACall.size() != 0;
    }

    auto FindModule() -> Pointer
    {
        Pointer pProcess = GetCurrentProcess();

        auto SnapShot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE , GetProcessId( pProcess ) );

        MODULEENTRY32 ModuleEntry;
        auto FirstModule = Module32First( SnapShot , &ModuleEntry );

        if ( IsPointer( FirstModule ) )
        {
            do
            {
                auto ModuleBase = reinterpret_cast< uintptr_t >( ModuleEntry.hModule );
                auto Function = reinterpret_cast< uintptr_t >( m_pFunction );

                if ( ( Function >= ModuleBase )
                     && ( Function < ( ModuleBase + ModuleEntry.modBaseSize ) ) )
                {
                    CloseHandle( SnapShot );

                    return ModuleEntry.hModule;
                }
            } while ( Module32Next( SnapShot , &ModuleEntry ) );
        }

        CloseHandle( SnapShot );

        return nullptr;
    }

    auto UnHook() -> bool
    {
        auto pDosHeader = ( PIMAGE_DOS_HEADER ) FindModule();

        if ( !IsPointer( pDosHeader ) )
            return false;

        for ( auto && FunctionRVACall : m_FoundFunctionsRVACall )
        {
            auto CallerAddress = reinterpret_cast< uintptr_t >( pDosHeader ) + FunctionRVACall;

            DWORD dwOldFlag;
            if ( VirtualProtect( reinterpret_cast< Pointer >( CallerAddress ) , sizeof( CallerAddress ) , PAGE_EXECUTE_READWRITE , &dwOldFlag ) )
            {
                uintptr_t *pWriteAddress = Write<uintptr_t>( CallerAddress );

                // Push back the old function.
                *pWriteAddress = reinterpret_cast< uintptr_t >( m_pOldRelativeRVA );

                if ( !VirtualProtect( reinterpret_cast< Pointer >( CallerAddress ) , sizeof( CallerAddress ) , dwOldFlag , &dwOldFlag ) )
                {
                    return false;
                }
            }
        }

        m_FoundFunctionsRVACall.clear();

        return true;
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
    constexpr FORCEINLINE auto CallFunction( vArgs ... pArgs )->RetType
    {
        return ( ( RetType( __cdecl* )( vArgs ... ) ) m_pFunction )( pArgs ... );
    }

private:
    std::vector<uintptr_t> m_FoundFunctionsRVACall;
    Pointer m_pOldRelativeRVA , m_pNewFunction , m_pFunction;
};
