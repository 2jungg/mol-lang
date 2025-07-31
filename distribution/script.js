const codeInput = document.getElementById('code-input');
const runButton = document.getElementById('run-button');
const output = document.getElementById('output');
const inputData = document.getElementById('input-data');

async function main() {
    output.textContent = 'Initializing Pyodide...';
    let pyodide = await loadPyodide();
    output.textContent += '\nPyodide loaded. Loading interpreter...';

    const interpreterCode = await fetch('interpreter.py').then(res => res.text());
    await pyodide.runPythonAsync(interpreterCode);
    
    output.textContent = 'Ready! Enter your Mol-Lang code and click Run.';

    runButton.addEventListener('click', async () => {
        const code = codeInput.value;
        const inputText = inputData.value;

        if (!code.trim()) {
            output.textContent = 'Please enter some code to run.';
            return;
        }
        
        output.textContent = 'Running...';
        try {
            const runMollang = pyodide.globals.get('run_mollang_code');
            const result = runMollang(code, inputText);
            output.textContent = result;
        } catch (error) {
            output.textContent = `An error occurred in JavaScript:\n${error}`;
        }
    });

    runButton.disabled = false;
    runButton.textContent = 'Run';
}

runButton.disabled = true;
runButton.textContent = 'Loading...';
main();
