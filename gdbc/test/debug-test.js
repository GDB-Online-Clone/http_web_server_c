import http from 'k6/http';
import { check, sleep } from 'k6';

/**
 * @description Default test configuration
 */
export const options = {
    vus: 10,        // Number of virtual users
    duration: '10s', // Test duration
    thresholds: {
        checks: ['rate>0.95'] // 95% of checks should pass
    }
};

/**
 * @description Simple C program for debugging
 */
const DEBUG_PROGRAM = `
#include <stdio.h>

int main() {
    printf("Hello, World!\\n");
    return 0;
}`;

/**
 * @description Request parameters for the test program
 */
const TEST_PARAMS = {
    source_code: DEBUG_PROGRAM,
    compiler_options: '',
    command_line_arguments: '',
    stdin: ''
};

/**
 * @description GDB commands to test debugging functionality
 */
const GDB_COMMANDS = [
    'break main',
    'run',
    'next',
    'print "Hello from GDB"',
    'continue',
    'quit'
].join('\n');

/**
 * @description Configuration for output polling
 */
const POLL_CONFIG = {
    maxAttempts: 15,    // Increased for debugger operations
    interval: 300,      // Polling interval in milliseconds
    timeout: 8000,      // Extended timeout for debugger operations
    commandDelay: 500   // Delay between GDB commands
};

/**
 * @description Executes a program in debugger mode
 * @param {Object} params Program execution parameters
 * @returns {Object} Response from the server
 */
function executeDebuggerProgram(params = TEST_PARAMS) {
    return http.post(
        'http://localhost:10010/run/debugger?language=c&compiler_type=gcc',
        JSON.stringify(params),
        {
            headers: { 'Content-Type': 'application/json' }
        }
    );
}

/**
 * @description Sends GDB commands to the running debugger
 * @param {number} pid Process ID
 * @param {string} commands GDB commands to execute
 * @returns {Object} Response from the server
 */
function sendDebuggerCommands(pid, commands) {
    return http.post(
        `http://localhost:10010/input?pid=${pid}`,
        JSON.stringify({ stdin: commands }),
        {
            headers: { 'Content-Type': 'application/json' }
        }
    );
}

/**
 * @description Checks debugger output with improved error handling
 * @param {number} pid Process ID to check
 * @param {Function} outputValidator Function to validate debugger output
 * @returns {Object} Object containing success status and captured output
 */
function pollDebuggerOutput(pid, outputValidator) {
    let attempts = 0;
    let startTime = new Date().getTime();
    let success = false;
    let lastOutput = '';
    let allOutput = '';

    while (attempts < POLL_CONFIG.maxAttempts) {
        const currentTime = new Date().getTime();
        if (currentTime - startTime > POLL_CONFIG.timeout) {
            console.log(`Polling timeout reached after ${POLL_CONFIG.timeout}ms`);
            break;
        }

        const response = http.get(`http://localhost:10010/program?pid=${pid}`);
        
        if (response.status === 204) {
            return { success, output: allOutput };
        }

        if (response.status === 200) {
            const newOutput = response.json('output');
            if (newOutput && newOutput !== lastOutput) {
                lastOutput = newOutput;
                allOutput += newOutput;
                if (outputValidator(allOutput)) {
                    success = true;
                }
            }
        }

        attempts++;
        sleep(POLL_CONFIG.interval / 1000);
    }

    return { success, output: allOutput };
}

/**
 * @description Main test scenario for Debugger Mode
 */
export default function () {
    // Step 1: Start program in debugger mode
    const runResponse = executeDebuggerProgram();
    
    const runChecks = check(runResponse, {
        'debugger start request is successful': (r) => r.status === 200,
        'debugger response has pid': (r) => r.json('pid') !== undefined
    });

    if (!runChecks) {
        console.error('Failed to start debugger');
        return;
    }

    const pid = runResponse.json('pid');

    // Step 2: Wait for GDB prompt
    const initialResult = pollDebuggerOutput(
        pid,
        (output) => output.includes('(gdb)')
    );

    check(null, {
        'GDB prompt appears': () => initialResult.success
    });

    if (!initialResult.success) {
        console.error('GDB failed to start properly');
        return;
    }

    sleep(POLL_CONFIG.commandDelay / 1000);

    // Step 3: Send GDB commands
    const debugCommands = sendDebuggerCommands(pid, GDB_COMMANDS);
    
    check(debugCommands, {
        'debug commands sent successfully': (r) => r.status === 200
    });

    // Step 4: Check debug output
    const debugResult = pollDebuggerOutput(
        pid,
        (output) => {
            return output.includes('Breakpoint 1, main')     // Check if breakpoint was hit
                && output.includes('Hello from GDB')         // Check GDB print command
                && output.includes('Hello, World!');         // Check program output
        }
    );

    check(null, {
        'debug session produced expected output': () => debugResult.success
    });
}

/**
 * @description Helper function to run custom debugging tests
 * @param {string} sourceCode C source code to debug
 * @param {string} gdbCommands GDB commands to execute
 * @param {Function} outputValidator Function to validate debug output
 * @returns {boolean} True if test passed
 */
export function runCustomDebugTest(sourceCode, gdbCommands, outputValidator) {
    const params = {
        ...TEST_PARAMS,
        source_code: sourceCode
    };

    const response = executeDebuggerProgram(params);
    if (!check(response, {
        'custom debug session start successful': (r) => r.status === 200
    })) {
        return false;
    }

    const pid = response.json('pid');

    // Wait for GDB prompt
    const promptResult = pollDebuggerOutput(
        pid,
        (output) => output.includes('(gdb)')
    );
    if (!promptResult.success) return false;

    sleep(POLL_CONFIG.commandDelay / 1000);

    // Send debug commands
    const commandResponse = sendDebuggerCommands(pid, gdbCommands);
    if (!check(commandResponse, {
        'debug commands sent successfully': (r) => r.status === 200
    })) {
        return false;
    }

    // Check debug output
    const debugResult = pollDebuggerOutput(pid, outputValidator);
    return debugResult.success;
}