const vscode = require('vscode');

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
    async function runBlades(checkOnly) {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showErrorMessage("Abra um arquivo .bl primeiro!");
            return;
        }

        const document = editor.document;
        const filePath = document.fileName;
        await document.save();
        
        // Lê a configuração do executável do Blades
        const config = vscode.workspace.getConfiguration('blades');
        let bladesExe = config.get('executablePath');
        
        if (!bladesExe || bladesExe.trim() === '') {
            // The release build is expected to expose `blades` through PATH.
            bladesExe = "blades";
        }

        // Cria ou reutiliza o terminal do Blades
        const terminalName = "Blades Engine";
        let terminal = vscode.window.terminals.find(t => t.name === terminalName);
        if (!terminal) {
            terminal = vscode.window.createTerminal(terminalName);
        }
        
        terminal.show();
        
        // Executa ou apenas valida o script.
        const executable = bladesExe.includes('\\') || bladesExe.includes('/')
            ? `& "${bladesExe}"`
            : bladesExe;
        const command = checkOnly ? '--check' : '';
        terminal.sendText(`${executable} ${command} "${filePath}"`.replace(/  +/g, ' '));
    }

    const runDisposable = vscode.commands.registerCommand('blades.run', () => runBlades(false));
    const checkDisposable = vscode.commands.registerCommand('blades.check', () => runBlades(true));

    context.subscriptions.push(runDisposable, checkDisposable);
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
}
