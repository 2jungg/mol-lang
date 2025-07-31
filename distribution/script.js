const codeInput = document.getElementById('code-input');
const runButton = document.getElementById('run-button');
const output = document.getElementById('output');
const inputData = document.getElementById('input-data');
const submitButton = document.getElementById('submit-button');

async function main() {
    output.textContent = 'Initializing Pyodide...';
    let pyodide = await loadPyodide();
    output.textContent += '\nPyodide loaded. Loading interpreter...';

    const interpreterCode = await fetch('interpreter.py').then(res => res.text());
    await pyodide.runPythonAsync(interpreterCode);
    
    output.textContent = 'Ready! Enter your Mol-Lang code and click Run.';

    let codeToRun = '';
    let isWaitingForInput = false;

    function executeCode(input = '') {
        if (!codeToRun.trim()) {
            output.textContent = 'Please enter some code to run.';
            return;
        }
        output.textContent = 'Running...';
        try {
            const runMollang = pyodide.globals.get('run_mollang_code');
            const result = runMollang(codeToRun, input);
            output.textContent = result;
        } catch (error) {
            output.textContent = `An error occurred in JavaScript:\n${error}`;
        }
        // Reset state
        isWaitingForInput = false;
        submitButton.disabled = true;
        runButton.disabled = false;
        inputData.value = '';
    }

    runButton.addEventListener('click', async () => {
        codeToRun = codeInput.value;
        if (!codeToRun.trim()) {
            output.textContent = 'Please enter some code to run.';
            return;
        }

        // Check for the specific input keyword '뭐먹'
        if (codeToRun.includes('뭐먹')) {
            output.textContent = "Code requires input. Please provide input below and click 'Submit'.";
            isWaitingForInput = true;
            submitButton.disabled = false;
            runButton.disabled = true; // Disable run until submission
        } else {
            executeCode(); // No input needed, run immediately
        }
    });

    submitButton.addEventListener('click', () => {
        if (isWaitingForInput) {
            executeCode(inputData.value);
        }
    });

    runButton.disabled = false;
    runButton.textContent = 'Run';
    submitButton.disabled = true; // Disable initially
}

runButton.disabled = true;
runButton.textContent = 'Loading...';
main();
