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
 * @description Interactive C program that prompts for user input
 */
const INTERACTIVE_PROGRAM = `
#include <stdio.h>

int main() {
    char name[100];
    printf("What's your name? ");
    fflush(stdout);
    scanf("%s", name);
    printf("Hello, %s!\\n", name);
    fflush(stdout);
    return 0;
}`;

/**
 * @description Request parameters for the test program
 */
const TEST_PARAMS = {
    source_code: INTERACTIVE_PROGRAM,
    compiler_options: '',
    command_line_arguments: '',
    stdin: ''
};

/**
 * @description Configuration for output polling
 */
const POLL_CONFIG = {
    maxAttempts: 10,    // Maximum number of polling attempts
    interval: 300,      // Polling interval in milliseconds
    timeout: 5000,      // Total timeout in milliseconds
    inputDelay: 200     // Delay before sending input (milliseconds)
};

/**
 * @description Executes a program through the interactive-mode endpoint
 * @param {Object} params Program execution parameters
 * @returns {Object} Response from the server
 */
function executeProgram(params = TEST_PARAMS) {
    return http.post(
        'http://localhost:10010/run/interactive-mode?language=c&compiler_type=gcc',
        JSON.stringify(params),
        {
            headers: { 'Content-Type': 'application/json' }
        }
    );
}

/**
 * @description Sends input to a running program
 * @param {number} pid Process ID
 * @param {string} input Input to send
 * @returns {Object} Response from the server
 */
function sendProgramInput(pid, input) {
    return http.post(
        `http://localhost:10010/input?pid=${pid}`,
        JSON.stringify({ stdin: input }),
        {
            headers: { 'Content-Type': 'application/json' }
        }
    );
}

/**
 * @description Checks program output with improved error handling and timeout
 * @param {number} pid Process ID to check
 * @param {Function} outputValidator Function to validate program output
 * @returns {Object} Object containing success status and captured output
 */
function pollProgramOutput(pid, outputValidator) {
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
            // Program completed
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
 * @description Main test scenario for Interactive Mode
 */
export default function () {
    // Step 1: Execute program
    const runResponse = executeProgram();
    
    const runChecks = check(runResponse, {
        'run request is successful': (r) => r.status === 200,
        'run response has pid': (r) => r.json('pid') !== undefined
    });

    if (!runChecks) {
        console.error('Failed to execute program');
        return;
    }

    const pid = runResponse.json('pid');

    // Step 2: Wait for initial prompt
    const initialResult = pollProgramOutput(
        pid,
        (output) => output.includes("What's your name?")
    );

    check(null, {
        'program prompts for input': () => initialResult.success
    });

    if (!initialResult.success) {
        console.error('Program failed to prompt for input');
        return;
    }

    sleep(POLL_CONFIG.inputDelay / 1000);

    // Step 3: Send input
    const inputResponse = sendProgramInput(pid, 'TestUser');
    
    check(inputResponse, {
        'input request is successful': (r) => r.status === 200
    });

    // Step 4: Check final output
    const finalResult = pollProgramOutput(
        pid,
        (output) => output.includes('Hello, TestUser!')
    );

    check(null, {
        'program produces expected output': () => finalResult.success
    });
}

/**
 * @description Helper function to run custom interactive tests
 * @param {string} sourceCode C source code to test
 * @param {string} input Input to send to the program
 * @param {string} expectedPrompt Expected prompt before input
 * @param {string} expectedOutput Expected output after input
 * @returns {boolean} True if test passed
 */
export function runCustomInteractiveTest(sourceCode, input, expectedPrompt, expectedOutput) {
    const params = {
        ...TEST_PARAMS,
        source_code: sourceCode
    };

    const response = executeProgram(params);
    if (!check(response, {
        'custom test execution successful': (r) => r.status === 200
    })) {
        return false;
    }

    const pid = response.json('pid');

    // Wait for prompt
    const promptResult = pollProgramOutput(pid, (output) => output.includes(expectedPrompt));
    if (!promptResult.success) return false;

    sleep(POLL_CONFIG.inputDelay / 1000);

    // Send input
    const inputResponse = sendProgramInput(pid, input);
    if (!check(inputResponse, {
        'input sent successfully': (r) => r.status === 200
    })) {
        return false;
    }

    // Check final output
    const finalResult = pollProgramOutput(pid, (output) => output.includes(expectedOutput));
    return finalResult.success;
}