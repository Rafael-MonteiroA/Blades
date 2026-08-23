const vscode = require('vscode');

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
    let disposable = vscode.commands.registerCommand('blades.run', function () {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showErrorMessage("Abra um arquivo .bl primeiro!");
            return;
        }

        const document = editor.document;
        const filePath = document.fileName;
        
        // Lê a configuração do executável do Blades
        const config = vscode.workspace.getConfiguration('blades');
        let bladesExe = config.get('executablePath');
        
        if (!bladesExe || bladesExe.trim() === '') {
            // Caminho hardcoded temporário apenas enquanto você desenvolve.
            // Para o público, eles terão que configurar o caminho ou ter o blades no PATH.
            bladesExe = "C:\\Users\\rafab\\Downloads\\blades\\build\\bin\\Debug\\blades.exe";
        }

        // Cria ou reutiliza o terminal do Blades
        const terminalName = "Blades Engine";
        let terminal = vscode.window.terminals.find(t => t.name === terminalName);
        if (!terminal) {
            terminal = vscode.window.createTerminal(terminalName);
        }
        
        terminal.show();
        
        // Executa o script
        terminal.sendText(`& "${bladesExe}" "${filePath}"`);
    });

    context.subscriptions.push(disposable);
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
}
