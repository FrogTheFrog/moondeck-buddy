// Test program to verify EnvSharedMemory functionality
// This would be a separate test executable, not part of the main build

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QProcessEnvironment>
#include "utils/envsharedmemory.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== Testing EnvSharedMemory ===";
    
    // Set some test environment variables
    qputenv("APOLLO_TEST_VAR", "test_value_1");
    qputenv("SUNSHINE_TEST_VAR", "test_value_2");
    qputenv("OTHER_VAR", "should_not_be_captured");
    
    qDebug() << "Set test environment variables";
    
    // Test 1: Capture and store
    utils::EnvSharedMemory writer;
    bool success = writer.captureAndStoreEnvironment({"APOLLO", "SUNSHINE"});
    qDebug() << "Capture and store result:" << success;
    
    // Test 2: Check if data exists
    bool hasData = writer.hasValidData();
    qDebug() << "Has valid data:" << hasData;
    
    // Test 3: Retrieve from different instance (simulating different process)
    utils::EnvSharedMemory reader;
    auto envVars = reader.retrieveEnvironment();
    qDebug() << "Retrieved" << envVars.size() << "environment variables:";
    for (auto it = envVars.constBegin(); it != envVars.constEnd(); ++it)
    {
        qDebug() << "  " << it.key() << "=" << it.value();
    }
    
    // Test 4: Clear
    bool cleared = writer.clearEnvironment();
    qDebug() << "Clear result:" << cleared;
    
    // Test 5: Try to retrieve after clear
    auto emptyVars = reader.retrieveEnvironment();
    qDebug() << "After clear, retrieved" << emptyVars.size() << "variables";
    
    qDebug() << "=== Test Complete ===";
    
    return 0;
}
