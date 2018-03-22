import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.2

DefaultDialog {
    title: qsTr("Configure FIDO")
    minimumHeight: calcHeight()
    height: minimumHeight
    minimumWidth: calcWidth()
    width: minimumWidth

    onVisibilityChanged: timer.running = !visible

    property var device
    property bool hasPin
    property int pinRetries
    property string pinMessage
    property bool pinBlocked

    function calcWidth() {
        return stack.currentItem ? Math.max(
                                       350,
                                       stack.currentItem.implicitWidth + (margins * 2)) : 0
    }

    function calcHeight() {
        return stack.currentItem ? stack.currentItem.implicitHeight + (margins * 2) : 0
    }

    function load() {
        stack.push({
                       item: fidoOverview,
                       immediate: true,
                       replace: true
                   })

        function handlePinResp(resp) {
            hasPin = resp
            if (hasPin) {
                device.fido_pin_retries(handleRetriesResp)
            } else {
                pinMessage = qsTr("No PIN is set.")
                show()
            }
        }
        function handleRetriesResp(resp) {
            if (!resp.error) {
                pinRetries = resp.retries
                pinMessage = qsTr("A PIN is set, ") + pinRetries + qsTr(
                            " retries left.")
            } else {
                pinMessage = resp.error
                pinBlocked = (resp.error === 'PIN is blocked.')
            }
            show()
        }
        device.fido_has_pin(handlePinResp)
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: fidoOverview
        onCurrentItemChanged: {
            if (currentItem) {
                currentItem.forceActiveFocus()
            }
        }
    }

    Component {
        id: fidoOverview
        FidoOverview {
        }
    }

    Component {
        id: fidoChangePinDialog
        FidoChangePinDialog {
        }
    }

    Component {
        id: fidoResetDialog
        FidoResetDialog {
        }
    }
}