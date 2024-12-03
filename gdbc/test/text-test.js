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
 * @description Basic C program that prints "Hello, World!"
 */
const HELLO_WORLD_PROGRAM = `
#include <stdio.h>

int main() {
    printf("Hello, World!\\n");
    fflush(stdout);  // Ensure immediate output
    return 0;
}`;

/**
 * @description Request parameters for the test program
 */
const TEST_PARAMS = {
    source_code: HELLO_WORLD_PROGRAM,
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
    timeout: 5000      // Total timeout in milliseconds
};

/**
 * @description Executes a program through the text-mode endpoint
 * @param {Object} params Program execution parameters
 * @returns {Object} Response from the server
 */
function executeProgram(params = TEST_PARAMS) {
    return http.post(
        'http://localhost:10010/run/text-mode?language=c&compiler_type=gcc',
        JSON.stringify(params),
        {
            headers: { 'Content-Type': 'application/json' }
        }
    );
}

/**
 * @description Checks program output by polling the /program endpoint with improved reliability
 * @param {number} pid Process ID to check
 * @param {Function} outputValidator Function to validate program output
 * @returns {boolean} True if program completed successfully
 */
function pollProgramOutput(pid, outputValidator) {
    let attempts = 0;
    let startTime = new Date().getTime();
    let success = false;
    let lastOutput = '';

    while (attempts < POLL_CONFIG.maxAttempts) {
        const currentTime = new Date().getTime();
        if (currentTime - startTime > POLL_CONFIG.timeout) {
            console.log('Polling timeout reached');
            break;
        }

        const response = http.get(`http://localhost:10010/program?pid=${pid}`);
        
        if (response.status === 204) {
            // Program completed
            return success;
        }

        if (response.status === 200) {
            const output = response.json('output');
            if (output && output !== lastOutput) {
                lastOutput = output;
                if (outputValidator(output)) {
                    success = true;
                    // Continue polling to ensure program completion
                }
            }
        }

        attempts++;
        sleep(POLL_CONFIG.interval / 1000);
    }

    return success;
}

/**
 * @description Main test scenario for Text Mode
 */
export default function () {
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
    const outputSuccess = pollProgramOutput(
        pid,
        (output) => output.includes('Hello, World!')
    );

    check(null, {
        'output validation passed': () => outputSuccess
    });
}