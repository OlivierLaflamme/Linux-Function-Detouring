pCDetour DetourTest = nullptr;

void Test()
{
    printf( "Hello!\n" );
}

void new_Test()
{
    printf( "OoooOps!\n" );
    DetourTest->CallFunction();
}

int main()
{
    Test();

    DetourTest = new CDetour( ( Pointer ) Test , ( Pointer ) new_Test );

    DetourTest->Detour();

    Test();

    return 0;
}
